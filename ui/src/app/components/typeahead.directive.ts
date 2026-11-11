/*
 * This program is free software; you can use it, redistribute it
 * and / or modify it under the terms of the GNU General Public License
 * (GPL) as published by the Free Software Foundation; either version 3
 * of the License or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program, in a file called gpl.txt or license.txt.
 * If not, write to the Free Software Foundation Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307 USA
 */

import { Directive, ElementRef, inject, input, OnDestroy, output } from "@angular/core";

@Directive({
    selector: "input[uiTypeahead]",
    host: {
        autocomplete: "off",
        "(focus)": "open()",
        "(click)": "open()",
        "(input)": "open()",
        "(blur)": "close()",
        "(keydown)": "key($event)",
    },
})
export class TypeaheadDirective<T> implements OnDestroy {
    readonly uiTypeahead = input.required<readonly T[] | undefined>();
    readonly uiTypeaheadLabel = input<(item: T) => string>(String);
    readonly selectItem = output<T>();

    private readonly el: HTMLInputElement = inject(ElementRef).nativeElement;
    private menu?: HTMLElement;
    private list: T[] = [];
    private idx = 0;

    open() {
        this.close();
        const term = this.el.value.toLowerCase();
        const label = this.uiTypeaheadLabel();
        this.list = (this.uiTypeahead() ?? [])
            .filter((i) => label(i).toLowerCase().includes(term))
            .slice(0, 10);
        if (!this.list.length) {
            return
        }
        this.idx = 0;

        const m = (this.menu = document.createElement("div"));
        m.className = "dropdown-menu show";
        m.setAttribute("role", "listbox");
        Object.assign(m.style, {
            position: "absolute",
            zIndex: "1000",
            top: this.el.offsetTop + this.el.offsetHeight + "px",
            left: this.el.offsetLeft + "px",
            minWidth: this.el.offsetWidth + "px"
        });

        m.addEventListener("mousedown", (e) => e.preventDefault());

        this.list.forEach((item, i) => {
            const a = document.createElement("button");
            a.type = "button";
            a.className = "dropdown-item w-100 text-start";
            a.setAttribute("role", "option");
            const text = label(item);
            const p = term ? text.toLowerCase().indexOf(term) : -1;
            if (p < 0) {
                a.textContent = text;
            } else {
                const b = document.createElement("b");
                b.textContent = text.substring(p, p + term.length);
                a.append(text.substring(0, p), b, text.substring(p + term.length));
            }
            a.addEventListener("click", () => this.pick(i));
            m.append(a);
        });
        this.mark();
        this.el.after(m);
    }

    close() {
        this.menu?.remove();
        this.menu = undefined;
    }

    key(e: KeyboardEvent) {
        if (!this.menu) {
            if (e.key === "ArrowDown") {
                this.open();
            }
            return;
        }
        const n = this.list.length;
        switch (e.key) {
            case "Escape":
                this.close();
                break;
            case "Enter":
                e.preventDefault();
                this.pick(this.idx);
                break;
            case "ArrowDown":
            case "ArrowUp":
                e.preventDefault();
                this.idx = (this.idx + (e.key === "ArrowDown" ? 1 : n - 1)) % n;
                this.mark();
        }
    }

    ngOnDestroy() {
        this.close();
    }

    private pick(i: number) {
        const item = this.list[i];
        this.close();
        if (item !== undefined) {
            this.selectItem.emit(item);
        }
    }

    private mark() {
        this.idx = Math.min(this.idx, this.list.length - 1);
        Array.from(this.menu?.children ?? []).forEach((c, i) => {
            c.classList.toggle("active", i === this.idx);
            if (i === this.idx) {
                c.scrollIntoView({block: "nearest"});
            }
        });
    }
}
