import fs from "node:fs";
import { FFS } from "./index.js";

const ffs = new FFS();
const fullflash = fs.readFileSync("/tmp/1.bin");
await ffs.open(fullflash);

console.log(ffs.getPlatform());
console.log(ffs.getModel());
console.log(ffs.getIMEI());

console.log(ffs.stat("/FFS/Pictures"));

const blob = ffs.readFile("/FFS_C/ccq_vinfo.txt");
console.log(blob?.toString());

const blob2 = ffs.readFile("/ffs/pictures/stalker_wallpapers_194.jpg");
if (blob2)
    fs.writeFileSync("/tmp/1.jpg", blob2);

console.log(ffs.stat("/"));
console.log(ffs.readDir("/"));
console.log(ffs.readDir("/ffs_c"));
console.log(ffs.readDir("/ffs_c/browser"));

console.log("tree /");
console.dir(ffs.getFilesTree(), { depth: null });

ffs.close();
