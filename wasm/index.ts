import Module, { Options, Entry, MainModule, FFS as NativeFFS } from "#build/libffshit_wasm.js";

let libffshit!: MainModule;

export type FFSOpenOptions = Partial<Options> & {
    platform?: "auto" | "EGOLD_CE" | "SGOLD" | "SGOLD2" | "SGOLD2_ELKA";
}

export class FFS {
    private handle: NativeFFS | undefined;

    async open(buffer: Buffer, options: FFSOpenOptions = {}) {
        libffshit = libffshit ?? await Module();
        this.handle = this.handle ?? new libffshit.FFS();

        const ptr = libffshit._malloc(buffer.length);
        libffshit.HEAPU8.set(new Uint8Array(buffer.buffer, buffer.byteOffset, buffer.byteLength), ptr);
        try {
            this.handle.open(ptr, buffer.length, {
                isOldSearchAlgorithm: false,
                searchStartAddress: 0,
                platform: "auto",
                skipBroken: true,
                skipDuplicates: true,
                debug: false,
                verboseData: false,
                verboseHeaders: false,
                verboseProcessing: false,
                ...options
            });
        } finally {
            libffshit._free(ptr);
        }
    }

    close() {
        if (!this.handle)
            throw new Error("FFS is not opened");
        this.handle.close();
    }

    getPlatform() {
        if (!this.handle)
            throw new Error("FFS is not opened");
        return this.handle.getPlatform();
    }

    getModel() {
        if (!this.handle)
            throw new Error("FFS is not opened");
        return this.handle.getModel();
    }

    getIMEI() {
        if (!this.handle)
            throw new Error("FFS is not opened");
        return this.handle.getIMEI();
    }

    stat(path: string): Entry | undefined {
        if (!this.handle)
            throw new Error("FFS is not opened");
        const entry = this.handle.stat(path);
        return entry.path == "" ? undefined : entry;
    }

    isExists(path: string) {
        return this.stat(path) != null;
    }

    readFile(path: string): Buffer | undefined {
        if (!this.handle)
            throw new Error("FFS is not opened");
        const result = this.handle.readFile(path);
        if (result.data) {
            const buffer = Buffer.from(new Uint8Array(libffshit.HEAPU8.buffer, result.data, result.size));
            libffshit._free(result.data);
            return buffer;
        }
    }

    readDir(path: string): Entry[] {
        if (!this.handle)
            throw new Error("FFS is not opened");
        const vector = this.handle.readDir(path);
        const entries: Entry[] = [];
        for (let i = 0, l = vector.size(); i < l; i++)
            entries.push(vector.get(i)!);
        return entries;
    }
};
