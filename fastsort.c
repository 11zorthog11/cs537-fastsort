#include <stdio.h> /* for printf */
#include <stdlib.h> /* for exit */
#include <unistd.h> /* for getopt */
#include <string.h> /* for strdup -- string duplicate */
#include <fcntl.h> /* for open, fstat */
#include <sys/types.h>
#include <sys/stat.h>
#include "sort.h" /* for rec_t struct */

int CompareArrays(const void* arr1, const void* arr2);
int calcNumLines(int fd);

void usage()
{
  fprintf(stderr, "Usage: fastsort -i inputfile -o outputfile\n");
  exit(1);
}

// external vars
int i;

int main(int argc, char *argv[])
{
  // check for correct number of arguments and correct flags
  if (argc != 5 || strcmp(argv[1], "-i") != 0 || strcmp(argv[3], "-o") != 0) {
    usage();
  }
  // arguments
  char *inFile = "/no/such/infile";
  char *outFile = "no/such/outfile";

  // input params
  int c;
  opterr = 0;
  while ((c = getopt(argc, argv, "i:o:")) != -1) {
    switch (c) {
    case 'i':
      inFile = strdup(optarg);
      break;
    case 'o':
      outFile = strdup(optarg);
      break;
    default:
      usage();
    }
  }

  // open input file
  int fd = open(inFile, O_RDONLY); // file descriptor
  // check for errors opening file
  if (fd < 0) {
    fprintf(stderr, "Error: Cannot open file %s\n", inFile);
    exit(1);
  }

  int numLines = calcNumLines(fd);
  /* Put keys and values in a 2D array of the format:
     [key1][rec1][rec2]...[rec24]
     [key2]...
     ...
     [keyN][rec1][rec2]...[rec24]
  */
  unsigned int keysAndRecs[numLines][NUMRECS + 1];
  
  // read input from file
  rec_t r;
  int currLine = 0;
  while(1) {
    int rc;
	rc = read(fd, &r, sizeof(rec_t));
	if (rc == 0) // 0 indicates EOF
	    break;
	if (rc < 0) {
	    perror("read");
	    exit(1);
	}
	// key value is in r.key
	keysAndRecs[currLine][0] = r.key;
	int j;
	for (j = 1; j <= NUMRECS; j++) {
	// rec values are in r.record
	  keysAndRecs[currLine][j] = r.record[j - 1];
	}
	currLine++;
  }

  // sort array by first column using the C library qsort
  void* arrPtr = (void*)&keysAndRecs;
  qsort(arrPtr, numLines, sizeof(unsigned int[NUMRECS + 1]), CompareArrays);
  
  (void) close(fd); // close input file

  // open and create output file
  fd = open(outFile, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
  if (fd < 0) {
    fprintf(stderr, "Error: Cannot open file %s\n", outFile);
    exit(1);
  }
  rec_t r2;
  int m;
  for (m = 0; m < numLines; m++) {
    // fill in the key
    r2.key = keysAndRecs[m][0];
    // fill in the recs
    int n;
    for (n = 0; n < NUMRECS; n++) {
      r2.record[n] = keysAndRecs[m][n + 1];
    }
    // write the rec_t info to the file
    int rc = write(fd, &r2, sizeof(rec_t));
    if (rc != sizeof(rec_t)) {
      fprintf(stderr, "Error: data of incorrect size written to file");
      (void) close(fd);
      exit(1);
    }
  }
  (void) close(fd);
  return(0);
}

/* Assumes that a file is open and that each line in 
   the file is 100 bytes. Returns the number of lines
   in the file.  It will only be an accurate count if
   the file meets the above assumptions.
 */
int calcNumLines(int fd){
 // determine size of file
  struct stat fileStat;
  fstat(fd, &fileStat);
  
  int fileSize = fileStat.st_size;
  return fileSize/100;
}

/* Custom comparator for qsort()
   Compares two arrays by comparing only the first 
   entries--the keys in this case.
 */
int CompareArrays(const void* arr1, const void* arr2) {
  // Convert to proper type
  const int* one = (const int*) arr1;
  const int* two = (const int*) arr2;
  // if the first key is less than the second key
  if (one[0] < two[0]) return -1;
  // if the first key is greater than the second key
  if (one[0] > two[0]) return 1;
  // both keys are equal
  return 0;
}
