/* SystemJS module definition */
declare let module: NodeModule;
interface NodeModule {
    id: string;
}

declare module "*.json" {
    const value: any;
    export default value;
}
