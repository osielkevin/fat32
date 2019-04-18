/*
Name: Kevin Lopez
*/
//#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/stat.h>
#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five argument
//File pointer to fat32
FILE *fp = NULL;
//File pointer to writing file
FILE *fpw;
//File pointer to puting
FILE *fpp;
//Checking pointer
FILE *checkingg = NULL;
//Variables to store our fat and beginning of reading of file
char BS_OEMName[8];
int16_t BPB_BytsPerSec;
int8_t BPB_SecPerClus;
int16_t BPB_RsvdSecCnt;
int8_t BPB_NumFATs;
int16_t BPB_RootEntCnt;
char BS_VolLab[11];
int32_t BPB_FATSz32;
int32_t BPB_RootClus;
int old_offset;
//Structure for containing clusters
struct __attribute__ ((__packed__)) DirectoryEntry 
   {
     char DIR_Name[11];
     uint8_t DIR_Attr;
     uint8_t Unused1[8];
     uint16_t DIR_FirstClusterHigh;
     uint8_t Unused2[4];
     uint16_t DIR_FirstClusterLow;
     uint32_t DIR_FileSize;
   };
//Number of directories can store apporixmately 512bytes
struct DirectoryEntry directory[16];
int32_t RootDirSectors = 0;
int32_t FirstDataSector = 0;
int32_t FirstSectorofCluster = 0;

//Calculates where the information starts in the file
int LBAToOffset(int32_t sector)
{
    return ((sector -2 ) * BPB_BytsPerSec) + (BPB_BytsPerSec * BPB_RsvdSecCnt) + (BPB_NumFATs* BPB_FATSz32 * BPB_BytsPerSec);
}
//Gets you the next logical block associated with the information
int16_t NextLB(uint32_t sector)
{
    uint32_t FatAddress = (BPB_BytsPerSec * BPB_RsvdSecCnt) + (sector * 4);
    int16_t val;
    fseek(fp,FatAddress,SEEK_SET);
    fread(&val, 2, 1, fp);
    return val;
}
//Function used to match fat32 format with regular computer commands
int find(char IMG_Name[], char input[],int j)
{
    if(input[0] == '.')
    {
        int a;
        int matches = 0;
        for(a = 0; a < strlen(input); a++)
        {
            if(input[a] == '.' && IMG_Name[a] == '.')
            {
                matches++;
            }
        }
        if(matches == strlen(input))
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

    char expanded_name[12];
    memset( expanded_name, ' ', 12 );

    char *token1 = strtok( input, "." );

    strncpy( expanded_name, token1, strlen( token1 ) );

    token1 = strtok( NULL, "." );
    if( token1 )
    {
        strncpy( (char*)(expanded_name+8), token1, strlen(token1) );
    }
    expanded_name[11] = '\0';

    int i;
    for( i = 0; i < 11; i++ )
    {
        expanded_name[i] = toupper( expanded_name[i] );
    }

  if( strncmp( expanded_name, IMG_Name, 11 ) == 0 )
  {
    return 1;
  }
  return 0;
}
int main()
{
    //These bytes are used for comparison purposes 
  unsigned int byte1 = 0x00;
  char  aaa;
  int32_t history = 0;
  unsigned int byte2 = 0xE5;
  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
  //Set this to false meaning there's no file open
  int is_file_open = 0;
  //Variables for loops
  int i = 0;
  int j = 0;
  int k = 0; 
  int32_t depth = 0;
  int l = 0;
  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }
    /*VVVVVVVVVVVVVVVVVVVVVVV CODE HERE VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
    /*
    ------------------------->OPEN
    */
    if(strcmp(token[0], "open") == 0)
    {
      //If file already open send error
      if(is_file_open  == 1)
      {
        printf("Error: File system image already open\n");

      }
      //File not open further check to see if the file is able to open
      else if(is_file_open == 0)
      {
        fp = fopen(token[1], "r+");
        //If file not open
        if(fp == NULL)
        {
            printf("Error: File system image not found.\n");
        }

        //Everything is correct so open file and change boolean to true
        else
        {
            is_file_open = 1;
            //Reading in the BS_OEMName
            fseek(fp,3,SEEK_SET);
            fread(&BS_OEMName,1,8,fp);
            //Reading Bytespersec
            fseek(fp,11,SEEK_SET);
            fread(&BPB_BytsPerSec,1,2,fp);
            //Reading Secperclus
            fseek(fp,13,SEEK_SET);
            fread(&BPB_SecPerClus,1,1,fp);
            //Reading RsvdSecCnt
            fseek(fp,14,SEEK_SET);
            fread(&BPB_RsvdSecCnt,1,2,fp);
            //Reading NumofFat
            fseek(fp,16,SEEK_SET);
            fread(&BPB_NumFATs,1,1,fp);
            //Reading BPB_RootEntCnt
            fseek(fp,17,SEEK_SET);
            fread(&BPB_RootEntCnt,1,2,fp);
            //ReadingVolLab
            fseek(fp,71,SEEK_SET);
            fread(&BS_VolLab,1,11,fp);
            //FATZ32
            fseek(fp,36,SEEK_SET);
            fread(&BPB_FATSz32,1, 4,fp);
            //Reading RootClus
            fseek(fp,44,SEEK_SET);
            fread(&BPB_RootClus,1,4,fp);
            //Calculation for the root of the cluster
            RootDirSectors = (BPB_NumFATs* BPB_FATSz32 * BPB_BytsPerSec)+ (BPB_RsvdSecCnt * BPB_BytsPerSec);
            RootDirSectors = (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) + (BPB_RsvdSecCnt * BPB_BytsPerSec);
            //FirstDataSector = BPB_RE
            i = 0;
            j = 0;
            unsigned int byte = 0x00;
            for(i = 0; i < 16; i++)
            {
                fseek(fp,RootDirSectors + (i * 32),SEEK_SET);
                fread(&directory[i],1 , 32, fp);
            }
            old_offset = RootDirSectors;
        }

      }

    }
    //-------------------> User presses info
    else if(strcmp(token[0],"info") == 0)
    {
        //If file is open then continue with the code
        if(is_file_open == 1){
            //Prints information in hex first then Decimal
            printf("BPB_BytesPerSec: Hex: %X Decimal: %d\n",BPB_BytsPerSec,BPB_BytsPerSec);
            printf("BPB_SecPerClus: Hex: %X Decimal: %d\n",BPB_SecPerClus,BPB_SecPerClus);
            printf("BPB_RsvdSecCnt: Hex: %X Decimal: %d\n",BPB_RsvdSecCnt,BPB_RsvdSecCnt);
            printf("BPB_NumFATS: Hex: %X Decimal: %d\n",BPB_NumFATs,BPB_NumFATs);
            printf("BPB_FATSz32: Hex: %X Decimal: %d\n",BPB_FATSz32,BPB_FATSz32);
            
        }
        //If file is not open print error message
        else
        {
            printf("Error: File system image must be opened first.\n");

        }

    }
    //----------------->User Presses ls
    else if(strcmp(token[0],"ls") == 0){
        //File is open and we assume its the correct format.
        if(is_file_open == 1)
        {
            //What we print out.
            char word[12];
            //For loop used printing out the 16 directories in the fat 32.
            for(i = 0; i < 16; i++)
            {
                /*
                Print Under the following conditions
                1 and 2 have to be true
                1. Character Name cannot be deleted
                2. Must be either Read only, Describes a subdirectory, or an archive flag
                */
               //Needed to cast this since char has a low range
               byte1 =(unsigned char) directory[i].DIR_Name[0];
                if((byte1 != byte2) && (directory[i].DIR_Attr  == 0x01|| directory[i].DIR_Attr  == 0x10||directory[i].DIR_Attr  == 0x20))
                {
                    j = 0;
                    
                    
                    //If were a directory put ...
                    //We copy the string so we can add the null character for printing reasons.
                    strcpy(word,directory[i].DIR_Name);
                    //Keep adding dots till not equal
                    word[11] = '\0';
                    printf("%s\n",word);
                }
            }
        }
        //File not open so we print an error message.
        else
        {
            printf("Error: File system image must be opened first.\n");
        }

    }
    //------------------------>stat pathway
    else if(strcmp(token[0], "stat") == 0)
    {
        if(is_file_open == 1)
        {
        //What I'm sending as argument since code is buggy.
            char send[101];
            char printer[12];
            strcpy(send,token[1]);
            for(i = 0; i < 16; i++)
            {
                //If we find it we must evaluate further
                if((find(directory[i].DIR_Name,send,i))== 1)
                {
                     /*
                    Validation of Data
                    Print Under the following conditions
                    1 and 2 have to be true
                    1. Character Name cannot be deleted
                    2. Must be either Read only, Describes a subdirectory, or an archive flag
                    If 1 and 2 correct we print about the file
                    */
                   //Byte 1 used for comparison
                    byte1 =(unsigned char) directory[i].DIR_Name[0]; 
                    if(((byte1 != byte2) && (directory[i].DIR_Attr  == 0x01||directory[i].DIR_Attr  == 0x20)) )
                    {
                        //To get away with printing without corrupting file
                        strcpy(printer,directory[i].DIR_Name);
                        printer[11] = '\0';
                        printf("File: %s\n",printer);
                        //Print statement
                        printf("Attributes: %d\nFirst Cluster Low:%d\nFile Size: %d\n",directory[i].DIR_Attr,directory[i].DIR_FirstClusterLow,directory[i].DIR_FileSize); 
                        //Used for flag
                        i = 100; 
                        break;
                    }
                    //If directory we print the information about the directory
                    else if(directory[i].DIR_Attr  == 0x10)
                    {
                        //To get away with printing without corrupting file
                        strcpy(printer,directory[i].DIR_Name);
                        printer[11] = '\0';
                        printf("Directory: %s\n",printer);
                        //Print statement
                        printf("Attributes: %d\nFirst Cluster Low:%d\n",directory[i].DIR_Attr,directory[i].DIR_FirstClusterLow); 
                        //Use for flag
                        i = 100; 
                        break;
                    }
                    i = 100;
                    break;
                 }
                 //copy string in for loop since function is buggy
                strcpy(send,token[1]);
            }
            if(i != 100)
            {
                printf("Error: File not found.\n");
            }   
        }
        //File not open so we print an error message.
        else
        {
            printf("Error: File system image must be opened first.\n");
        }

    }
    //-------------------------> read pathway
    else if (strcmp(token[0], "read") == 0)
    {
        if(is_file_open == 1)
        {
            //What I'm sending as argument since code is buggy.
            char send[101];
            char printer[12];
            strcpy(send,token[1]);
            for(i = 0; i < 16; i++)
            {
                //If we find it we must evaluate further
                if((find(directory[i].DIR_Name,send,i))== 1)
                {
                     /*
                    Validation of Data
                    Print Under the following conditions
                    1 and 2 have to be true
                    1. Character Name cannot be deleted
                    2. Must be either Read only, Describes a subdirectory, or an archive flag
                    If 1 and 2 correct we print about the file
                    */
                    byte1 =(unsigned char) directory[i].DIR_Name[0];
                    if(((byte1 != byte2) && 
                    (directory[i].DIR_Attr  == 0x01|| directory[i].DIR_Attr  == 0x010
                    ||directory[i].DIR_Attr  == 0x20)) )
                    {

                        strcpy(printer,directory[i].DIR_Name);
                        printer[11] = '\0';
                        char letter;
                        int check = 0;
                        printf("File: %s\n",printer);
                        //This tells us where the information starts
                        int offset = LBAToOffset(directory[i].DIR_FirstClusterLow);
                        //Convert these numbers for the loops in ascii
                        int position  = atoi(token[2]);
                        int num_of_byte = atoi(token[3]);
                        //We need the logical block and we made a copy
                        //We don't want to override the original data
                        //Changed hereVVVVVV
                        int16_t copy = directory[i].DIR_FirstClusterLow;
                        //We need this as a float so we can evaluate loop
                        float checker = (float)position;
                        //display 
                        uint8_t display = 0x00;
                        //Number of bytes that it should go to
                        for(j = 0; j < num_of_byte; j++)
                        {
                            //If the position is greater then go to a subroutine
                            if(position>= 512)
                            {
                                //Tells how many times to partion ration must be greater than 1
                                //If it's greater than 1 we know for sure we don't need to look
                                //For the next block
                                checker = (float)position;
                                while((512 / checker) <= 1 )
                                {
                                //We ovveride the old block
                                 copy = NextLB(copy);
                                 //Get a new offset
                                 offset = LBAToOffset(copy);
                                 //Adjust the position so we don't go somewhere else
                                 position = position - 512; 
                                 //Make sure to change the comparison for the while
                                 checker /= 512;
                                }
                            }
                            //With the arithmetic done we move to spot
                            fseek(fp,(position + offset ),SEEK_SET);
                            //Capture byte by byte
                            fread(&display,1,1,fp);
                            //Print out the byte we previously captured
                            printf("%X",display);
                            //Increment the position since were no longer there
                            position++;
                        }
                        //Used for flag
                    }
                    printf("\n");
                    break;
                }
                //Copy to avoid data corruption
                strcpy(send,token[1]);
            }
        }
        //Nothing in the file so we can't do the code
        else
        {
            //Generic error message
            printf("Error: File system image must be opened first.\n");
        }
    }
    //----------->get code
    else if(strcmp(token[0], "get")== 0)
    {
        //If file is open 
        if(is_file_open == 1)
        {
            //What we compare with
            char send[101];
            strcpy(send,token[1]);
            for(i = 0; i < 16; i++)
            {
                //If we find the file then must validate that it is a file.
                if(find(directory[i].DIR_Name,send,i) == 1)
                {
                    /* Validation of Data
                    Print Under the following conditions
                    1 and 2 have to be true
                    1. Character Name cannot be deleted
                    2. Must be either Read only, Describes a subdirectory, or an archive flag
                    3. Cannot be a directory strictly file
                    If 1 and 2 correct we print about the file
                    */
                   //Used for comaparison
                   byte1 =(unsigned char) directory[i].DIR_Name[0];
                    if(((byte1 != 0xE5)  && (directory[i].DIR_Attr  == 0x01||
                     directory[i].DIR_Attr  == 0x10||directory[i].DIR_Attr  == 0x20)))
                    {
                        //We open file and begin writing to it
                        fpw = fopen(token[1], "w");
                        int size = directory[i].DIR_FileSize;
                        j = 0;
                        //Extra variable so not to get confused with overriding numbers
                        int position = 0;
                        //Where we store the byte
                        char letter;
                        //Copy of LBA
                        int16_t copy = directory[i].DIR_FirstClusterLow;
                        //This tells us where the information starts
                        int offset1 = LBAToOffset(directory[i].DIR_FirstClusterLow);
                        //container for sending
                        uint8_t display = 0x00;
                        //Keep writing to file as long as the file exists in length
                        while(j < size)
                        {
                            //If your position on the file read is 512 byte need to find lba
                            if(position == 512 )
                            {
                                copy = NextLB(copy);
                                offset1 = LBAToOffset(copy);
                                //Reset position since were at new block
                                position = 0;
                            }
                            //Move cursor at approriate place
                            fseek(fp,(offset1 + position), SEEK_SET);
                            //Move the byte to that one character
                            fread(&display,1,1,fp);
                            fwrite(&display,sizeof(display),1,fpw);
                            position++;
                            j++;
                        }
                        //Close the file
                        fclose(fpw);
                        //Break can only be one file
                        break;

                    }
                    //It's correct what they typed but it wasnt a file

                }
                strcpy(send,token[1]);
            }
            //Generic error message because it didnt find the file or it wasnt a file
            if(i == 16)
            {
                printf("Error: File not found\n");
            }

        }
        //Print error messaage since file is not open
        else
        {
            //Generic error message
            printf("Error: File system image must be opened first.\n");
        }

    }
    //Change the directory 
    else if(strcmp(token[0],"cd") == 0)
    {
        if(is_file_open == 1)
        {
            char send[200];
            char og[200];
            //Pieces it up if /
            char *iterator = strtok(token[1],"/");
            //do all commands til /
            while(iterator!=NULL)
            {
                //put iterator to og so it wont override
                strcpy(og,iterator);
                //iterate
                iterator = strtok(NULL,"/");
                for(i = 0; i < 16; i++)
                {
                    //since code buggy override
                    strcpy(send, og);
                    //if you find it check over it
                    if(find(directory[i].DIR_Name,send,1) == 1)
                    {
                        //if its not a directory get out
                        if(directory[i].DIR_Attr != 0x10)
                        {
                            break;
                        }
                        //Offset of where the information starts and write down the struct
                        int offset = LBAToOffset(directory[i].DIR_FirstClusterLow);
                        old_offset = offset;
                        //If we shot out of bounds then move to root
                        if(offset <= RootDirSectors)
                        {
                            offset = RootDirSectors;
                            old_offset = RootDirSectors;
                        }
                        for(j = 0; j < 16; j++)
                        {
                            //offset of where to put the struct
                            fseek(fp,offset + (j * 32),SEEK_SET);
                            fread(&directory[j],1 , 32, fp);
                        }
                        break;
                    }
                }
            }
        }
        //If file not open then erase 
        else
        {
            printf("Error: File system image must be opened first.\n");
        }
    }
    else if(strcmp(token[0],"close") == 0)
    {
        //If we have something open close it and reset the boolean
        if(is_file_open == 1)
        {
            fclose(fp);
            is_file_open = 0;
        }
        else
        {
            printf("Error: File system image must be opened first.\n");
        }

    }//------------>If they press put go here
    else if(strcmp(token[0], "put") == 0)
    {
        if(is_file_open == 0){
            printf("Error: File system image must be opened first.\n");
            continue;
        }
        int flag = -1;
        struct stat st;
        int c_heck = stat(token[1], &st);
        int size_of_file = st.st_size;
        if(is_file_open == 1)
        {
            //If the file is not existent then print error and iterate
             if(c_heck == -1)
            {      
                //close the file
                fclose(checkingg);
                printf("Importing file not in the system\n");
                continue;
            }
            else
            {
                //open the file so we can check if theres something in there
            checkingg = fopen(token[1], "r");
                /*VFILLLE COOOODE VVVVVV*/
                //String of 12 that we will manipulate added for the null character
                char word[12];
                //Made copy for string manipulation
                char dupe [102];
                //Made copy for string manipulation
                strcpy(dupe, token[1]);
                //Filling up the string with spaces
                for(i = 0; i < 11; i++)
                word[i] = ' ';
                //Converting dupe to uppercase
                for(i = 0; i < strlen(dupe); i++)
                 dupe[i] = toupper(dupe[i]);
                 //Reset i to zero for conventional reasons
                 i = 0;
                 //total variable helps keep track of number of letters that we insert
                 int total = 0;
                 //Boolean set to false but also keeps track of where dot occurs
                 int dot_position = -1;
                 //Go through word char
                 for(i = 0; i < strlen(dupe); i++)
                 {
                     //If we hit a dot save position and break
                     if(dupe[i] == '.')
                     {
                         dot_position = i;
                         break;
                     }
                     //If were in the loop and if the position is not a .
                     //Also if total is not 8 then capture
                     if(dupe[i] != '.' && total < 8)
                     {
                         //Copy and put what's needed
                         word[total] = dupe[i];
                         total++;
                     }
                 }
                 //If we never encountered a dot its a folder or invalid file name
                 if(dot_position == -1)
                 {
                     continue;
                 }
                 //We increment dot position  
                 dot_position++;
                 //Store the word that were sending to the fat32/struct
                 for(i = 8; i <= 10; i++)
                 {
                     word[i] = dupe[dot_position];
                     dot_position++;

                 }
                 //Look for empty space
                for (i = 0; i < 16; i++)
                {
                    byte1 =(unsigned char) directory[i].DIR_Name[0]; 
                    if(byte1 == byte2 || byte1 == 0x00)
                    {
                        /*
                        Ovveriding our structure 

                        */
                        for(j = 0; j < 11; j++)
                        {
                            //store the word format into the struct
                            directory[i].DIR_Name[j] = word[j];
                        }
                        //Set the attribute to 0x20
                        directory[i].DIR_Attr = 0x20;
                        //Set the filesize
                        directory[i].DIR_FileSize = size_of_file;
                        /*CodeVVVVVVV */
                        int32_t copy = 0x00;
                        int32_t copy1 = 0x00;
                        //What were reading and sending to fat
                        uint8_t storage[512];
                        //Pointer of where were at in the file
                        int where_in_file = 0;
                        //boolean flag signaling that were done
                        int flag1 = 0; 
                        //equation of what were doing 
                        int32_t equation_check = -3;
                        //Low cluster number 
                        uint16_t low_clus;
                        //low cluster number of what were sending to struct
                        uint16_t first_clus;
                        //boolean flag signaling we made a logical block
                        int first_1 = 0;
                        //offset info telling us where we can write
                        int offset_info = 0;
                        //base meaning start of fat
                        int base = BPB_RsvdSecCnt * BPB_BytsPerSec;
                        //start point telling us where to go
                        int start = BPB_RsvdSecCnt * BPB_BytsPerSec;
                        //Forward needed to know where to go ahead and check
                        int forward = start;
                        //End of fat
                        int end = BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec;
                        //run until we get out of the fat
                        while(start < end)
                        {
                            //Parse and check and if start is at 0
                            fseek(fp,start,SEEK_SET);
                            fread(&copy,1,4,fp);
                            //if start hits a 0 start parsing
                            if(copy == 0x00)
                            {
                                //move position by 32 bits of forward index                    
                                forward = start + 4;
                                //equation from lb reworked
                                //this will help us come up with a lb number
                                equation_check = forward - base;
                                fseek(fp,forward,SEEK_SET);
                                fread(&copy1,1,4,fp);
                                //if it doesn't contain a 0 or not an integer 
                                while(copy1 != 0x00 || equation_check % 4 != 0)
                                {
                                    //move by 32 bits
                                    forward+=4;
                                    //move the equation
                                    equation_check = forward - base;
                                    fseek(fp,forward,SEEK_SET);
                                    //check and see
                                    fread(&copy1,1,4,fp);
                                }
                                //generate the magic number
                                equation_check /= 4;
                                //go to fat32 so we can write the magic number
                                fseek(fp,start,SEEK_SET);
                                //Store it in fat32
                                fwrite(&equation_check,1,4,fp);
                                //traverse so we can go low cluster
                                fseek(fp,start,SEEK_SET);
                                //capture the low cluster
                                fread(&low_clus,1,2,fp);
                                //if first time getting cluster store it
                                if(first_1 == 0){
                                    //Store the first cluster
                                    first_clus = low_clus;
                                    //change the boolean
                                    first_1 = 1;
                                }
                                //decrement filesize
                                size_of_file-=512;
                                //get new offset of where to write
                                offset_info = LBAToOffset(low_clus);
                                //Write 512
                                    fseek(checkingg,where_in_file,SEEK_SET);
                                    fread(&storage,1,512,checkingg);
                                    fseek(fp,offset_info, SEEK_SET);
                                    fwrite(storage,sizeof(storage),1,fp);
                                    where_in_file+=512;
                                //we wrote all the file so we end the block and leave
                                if(size_of_file <= 0)
                                {
                                    flag1 = -1;
                                    fseek(fp,forward,SEEK_SET);
                                    fwrite(&flag,1,4,fp);
                                }
                                else
                                {
                                    //we have a link so we keep going
                                    //Switch the indexes of the start and forward inde
                                    start = forward;
                                    flag1 = 0;
                                    continue;
                                }
                                

                            }
                            //we end the block and write the struct into fat32
                            if(flag1 == -1){
                                directory[i].DIR_FirstClusterLow = first_clus;
                                fseek(fp,old_offset+ (i* 32),SEEK_SET);
                                fwrite(&directory[i],32,1,fp);
                                break;
                            }
                            //increment start
                            fseek(fp,start,SEEK_SET);
                            fread(&copy,1,4,fp);
                            start+=4;
                        }
                        //we filled up a slot so no need to keep going through loop
                        break;
                    }
                }
                
                /*^^^^^^^^^code here^^^^^^^*/

            }
            fclose(checkingg);
        }
        else
        {
            fclose(checkingg);
            printf("Error: File system image must be opened first.\n");
        }
    }
    /*^^^^^^^^^^^^^^^^^^^^^^CODE ABOVE HERE^^^^^^^^^^^^^^^^^^^^^^*/
    int token_index  = 0;
    free( working_root );

  }
  return 0;
}
