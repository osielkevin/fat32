Must use linux
to compile type: g++ mfs.c -o mfs

using makefile:
  Makefile make - Makes the file
  Makefile clean - cleans executable

Using Fat32
Must be on a linux type OS
Commands that work
-----MUST TYPE Open fat32.img /// TO EVEN USE if not commands will not work----
1. ls
2. cd <directory>
3. get <filename> - retrieves file from fat32 and stores it on the users computer
4. put <filename> - puts a file from user space and stores it to the fat32
5. read <starting position int> <final reading position int> - reads file from fat32 and spits it out in hex to console 
