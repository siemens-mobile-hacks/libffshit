import Module, { Options, Entry } from "#build/libffshit_wasm.js";

const libffshit = await Module();

export type FFSOpenOptions = Partial<Options> & {
    platform?: "auto" | "EGOLD_CE" | "SGOLD" | "SGOLD2" | "SGOLD2_ELKA";
}

export class FFS {
    private handle = new libffshit.FFS();

    constructor() {

    }

    open(buffer: Buffer, options: FFSOpenOptions = {}) {
        const ptr = libffshit._malloc(buffer.length);
        libffshit.HEAPU8.set(buffer, ptr);
        try {
            this.handle.open(ptr, buffer.length, {
                isOldSearchAlgorithm: false,
                searchStartAddress: 0,
                platform: "auto",
                skipBroken: true,
                skipDuplicates: true,
                ...options
            });
        } finally {
            libffshit._free(ptr);
        }
    }

    getPlatform() {
        return this.handle.getPlatform();
    }

    getModel() {
        return this.handle.getModel();
    }

    getIMEI() {
        return this.handle.getIMEI();
    }

    stat(path: string): Entry | undefined {
        const entry = this.handle.stat(path);
        return entry.path == "" ? undefined : entry;
    }

    isExists(path: string) {
        return this.stat(path) != null;
    }

    readFile(path: string): Buffer | undefined {
        const result = this.handle.readFile(path);
        return result.data ? Buffer.from(new Uint8Array(libffshit.HEAPU8.buffer, result.data, result.size)) : undefined;
    }

    readDir(path: string): Entry[] {
        const vector = this.handle.readDir(path);
        const entries: Entry[] = [];
        for (let i = 0, l = vector.size(); i < l; i++)
            entries.push(vector.get(i)!);
        return entries;
    }
};
