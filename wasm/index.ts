import Module, { Options, Entry, MainModule, FFS as NativeFFS } from "#build/libffshit_wasm.js";

interface MainModuleEx extends MainModule {
    getExceptionMessage(err: unknown): [string, string];
}

let libffshit!: MainModuleEx;

// C++ exceptions arrive as WebAssembly.Exception, which is not an Error.
// JS errors and traps (WebAssembly.RuntimeError) are passed through.
function toError(e: unknown): Error {
    if (e instanceof Error)
        return e;
    return new Error(libffshit.getExceptionMessage(e)[1]);
}

export type FFSOpenOptions = Partial<Options> & {
    platform?: "auto" | "EGOLD_CE" | "SGOLD" | "SGOLD2" | "SGOLD2_ELKA";
}

export type FFSEntry = Entry;

export type FFSTreeEntry = FFSEntry & {
    children?: FFSTreeEntry[];
};

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
        } catch (e) {
            throw toError(e);
        } finally {
            libffshit._free(ptr);
        }
    }

    close() {
        if (!this.handle)
            throw new Error("FFS is not opened");
        try {
            this.handle.close();
        } catch (e) {
            throw toError(e);
        }
    }

    getPlatform() {
        if (!this.handle)
            throw new Error("FFS is not opened");
        try {
            return this.handle.getPlatform();
        } catch (e) {
            throw toError(e);
        }
    }

    getModel() {
        if (!this.handle)
            throw new Error("FFS is not opened");
        try {
            return this.handle.getModel();
        } catch (e) {
            throw toError(e);
        }
    }

    getIMEI() {
        if (!this.handle)
            throw new Error("FFS is not opened");
        try {
            return this.handle.getIMEI();
        } catch (e) {
            throw toError(e);
        }
    }

    // Problems found while opening, e.g. broken files that were skipped
    getWarnings(): string[] {
        if (!this.handle)
            throw new Error("FFS is not opened");
        try {
            const vector = this.handle.getWarnings();
            const warnings: string[] = [];
            // Typed as any embind string input, but std::string always comes out as a string
            for (let i = 0, l = vector.size(); i < l; i++)
                warnings.push(vector.get(i) as string);
            vector.delete();
            return warnings;
        } catch (e) {
            throw toError(e);
        }
    }

    stat(path: string): FFSEntry | undefined {
        if (!this.handle)
            throw new Error("FFS is not opened");
        try {
            const entry = this.handle.stat(path);
            return entry.path == "" ? undefined : entry;
        } catch (e) {
            throw toError(e);
        }
    }

    isExists(path: string) {
        return this.stat(path) != null;
    }

    readFile(path: string): Buffer | undefined {
        if (!this.handle)
            throw new Error("FFS is not opened");
        try {
            const result = this.handle.readFile(path);
            if (result.data) {
                const buffer = Buffer.from(new Uint8Array(libffshit.HEAPU8.buffer, result.data, result.size));
                libffshit._free(result.data);
                return buffer;
            }
        } catch (e) {
            throw toError(e);
        }
    }

    readDir(path: string): FFSEntry[] {
        if (!this.handle)
            throw new Error("FFS is not opened");
        try {
            const vector = this.handle.readDir(path);
            const entries: FFSEntry[] = [];
            for (let i = 0, l = vector.size(); i < l; i++)
                entries.push(vector.get(i)!);
            return entries;
        } catch (e) {
            throw toError(e);
        }
    }

    getFilesTree(): FFSTreeEntry {
        const rootDirStat = this.stat("/");
        if (!rootDirStat)
            throw new Error("Root directory is not found");

        return {
            ...rootDirStat,
            children: this.readDirRecursive("/")
        };
    }

    readDirRecursive(path: string): FFSTreeEntry[] {
        const entries: FFSTreeEntry[] = [];
        for (const entry of this.readDir(path)) {
            if (entry.isDirectory) {
                entries.push({
                    ...entry,
                    children: this.readDirRecursive(entry.path + "/" + entry.name)
                });
            } else {
                entries.push({
                    ...entry,
                    children: []
                });
            }
        }
        return entries;
    }
}
