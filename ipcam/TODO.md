1. ~~Integrate redis with synchronous API and libevent to commit image
   binaries into redis and then process from there.~~
2. ~~Do not write the file.~~
3. ~~Use only jpeg_mem_dest from libjpeg_turbo, replacing jpeg_stdio_dest
   to create the jpg dest in memory before committing this dest into a
   redis queue.~~
4. Code cleanup, hunt and fix memory leaks.
5. Commit captures in two different streams, depending on whether we
   have double capture or not.
