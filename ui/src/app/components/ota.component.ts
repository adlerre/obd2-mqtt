/*
 * This program is free software; you can use it, redistribute it
 * and / or modify it under the terms of the GNU General Public License
 * (GPL) as published by the Free Software Foundation; either version 3
 * of the License or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program, in a file called gpl.txt or license.txt.
 *  If not, write to the Free Software Foundation Inc.,
 *  59 Temple Place - Suite 330, Boston, MA  02111-1307 USA
 */

import { Component, Directive, EventEmitter, HostListener, Output, ViewEncapsulation } from "@angular/core";
import { ApiService } from "../services/api.service";
import { distinctUntilChanged, Subject } from "rxjs";
import { NgbProgressbarModule } from "@ng-bootstrap/ng-bootstrap";
import { CommonModule } from "@angular/common";
import { FileItem, OTAMode } from "../definitions";
import { FormControl, ReactiveFormsModule } from "@angular/forms";

@Directive({standalone: true, selector: "[fileDropZone]"})
export class FileDropZoneDirective {

    @Output()
    public files = new EventEmitter();

    @HostListener("dragenter", ["$event"])
    @HostListener("dragover", ["$event"])
    onDragOver(event: any) {
        event.preventDefault();

        // TODO check if accept
    }

    @HostListener("drop", ["$event"])
    onDrop(event: any) {
        event.preventDefault();

        if (event.dataTransfer.items) {
            this.getFilesWebkitDataTransferItems(event.dataTransfer.items).then((files) => this.upload(files));
        } else {
            this.upload(this.toFileArray(event.dataTransfer.files));
        }
    }

    @HostListener("change", ["$event"])
    onChange(event: any) {
        event.preventDefault();

        if (event.target.webkitEntries && event.target.webkitEntries.length !== 0) {
            this.getFilesWebkitDataTransferItems(event.target.webkitEntries).then((files) => this.upload(files));
        } else {
            this.upload(this.toFileArray(event.target.files));
        }
    }

    private toFileArray(fileList: FileList): Array<File> {
        const files = [];
        if (fileList instanceof FileList) {
            for (let i = 0; i < fileList.length; i++) {
                files.push(fileList.item(i));
            }
        }
        return files;
    }

    private getFilesWebkitDataTransferItems(dataTransferItems: any) {
        const files = [];

        const traverseFileTreePromise = (item: any, path = "") => new Promise(resolve => {
            if (item.isFile) {
                item.file((file: any) => {
                    file.filepath = path + file.name;
                    files.push(file);
                    resolve(file);
                });
            } else if (item.isDirectory) {
                const dirReader = item.createReader();
                dirReader.readEntries((entries: any) => {
                    const entriesPromises = [];
                    for (const entr of entries) {
                        entriesPromises.push(traverseFileTreePromise(entr, path + item.name + "/"));
                    }
                    resolve(Promise.all(entriesPromises));
                });
            }
        });

        return new Promise((resolve, _reject) => {
            const entriesPromises = [];
            for (const it of dataTransferItems) {
                entriesPromises.push(traverseFileTreePromise(it.webkitGetAsEntry ? it.webkitGetAsEntry() : it));
            }
            Promise.all(entriesPromises).then(() => resolve(files));
        });
    }

    private upload(files: any) {
        this.files.next(files);
    }
}


@Component({
    selector: "ui-ota",
    templateUrl: "./ota.component.html",
    styleUrls: ["./ota.component.scss"],
    imports: [CommonModule, ReactiveFormsModule, NgbProgressbarModule, FileDropZoneDirective],
    encapsulation: ViewEncapsulation.None
})
export class OTAComponent {

    private static MAX_CONCURRENT_UPLOADS = 1;

    mode: FormControl = new FormControl(OTAMode.FIRMWARE);

    uploadEnabled = false;

    queue: Array<FileItem> = [];

    constructor(private $api: ApiService) {
    }

    onFiles(files: any) {
        if (files instanceof Array) {
            files.forEach((f: File) => {
                if (files.length > 1 &&
                    f.name.toLowerCase().indexOf("firmware") === -1 &&
                    f.name.toLowerCase().indexOf("littlefs") === -1) {
                    return;
                }

                const qitem: FileItem = {
                    file: f,
                    mode: files.length === 1 ?
                        null :
                        f.name.toLowerCase().indexOf("firmware") !== -1 ? OTAMode.FIRMWARE : OTAMode.LITTLE_FS,
                    progressEvent: new Subject(),
                    complete: false,
                    processing: false,
                    error: false,
                    created: Date.now()
                };
                qitem.progressEvent.pipe(distinctUntilChanged()).subscribe(p => qitem.progress = p);
                this.queue.push(qitem);
            });

            this.uploadEnabled = this.queue.length > 0;
            if (this.queue.length === 1) {
                const f = this.queue[0].file;
                this.mode.patchValue(
                    f.name.toLowerCase().indexOf("firmware") !== -1 ? OTAMode.FIRMWARE : OTAMode.LITTLE_FS
                );
            }
        }
    }

    onUploadClick(_evt: MouseEvent) {
        this.uploadEnabled = false;
        this.invokeUpload();
    }

    getModes(): Array<{ key: string, value: string }> {
        return Object.keys(OTAMode).map(key => ({
            key: key.replaceAll("_", " "),
            value: OTAMode[key as keyof typeof OTAMode]
        }));
    }

    private queueItemProcess(item: any) {
        if (item.complete === false && item.processing === false && item.error === false) {
            item.started = Date.now();
            item.processing = true;

            this.$api.otaStart(item.mode || this.mode.value).subscribe({
                next: () => this.$api.otaUpload(
                    item.file,
                    item.progressEvent
                )
                    .subscribe({
                        next: (res) => this.queueItemDone(item, res),
                        error: (err) => this.queueItemError(item, err)
                    }),
                error: (err) => this.queueItemError(item, err)
            });
        }
    }

    private queueItemDone(item: FileItem, res: any) {
        item.processing = false;
        item.complete = true;
        item.completed = Date.now();

        item.error = res.status !== 200;

        this.invokeUpload();
    }

    private queueItemError(item: FileItem, err: any) {
        item.processing = false;
        item.complete = true;
        item.error = true;

        if (err.error) {
            item.errorMsg = err.error;
        }
        console.error(err);

        this.invokeUpload();
    }

    private invokeUpload() {
        const waiting = this.queue.filter(i => i.complete === false && i.processing === false && i.error === false)
            .slice(0, OTAComponent.MAX_CONCURRENT_UPLOADS);

        if (waiting.length !== 0) {
            waiting.forEach((item) => {
                this.queueItemProcess(item);
            });
        }
    }


}
