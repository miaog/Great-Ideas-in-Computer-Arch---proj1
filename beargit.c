#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>

#include "beargit.h"
#include "util.h"
#include <ctype.h>
#include <stdlib.h>

/* Implementation Notes:
 *
 * - Functions return 0 if successful, 1 if there is an error.
 * - All error conditions in the function description need to be implemented
 *   and written to stderr. We catch some additional errors for you in main.c.
 * - Output to stdout needs to be exactly as specified in the function description.
 * - Only edit this file (beargit.c)
 * - Here are some of the helper functions from util.h:
 *   * fs_mkdir(dirname): create directory <dirname>
 *   * fs_rm(filename): delete file <filename>
 *   * fs_mv(src,dst): move file <src> to <dst>, overwriting <dst> if it exists
 *   * fs_cp(src,dst): copy file <src> to <dst>, overwriting <dst> if it exists
 *   * write_string_to_file(filename,str): write <str> to filename (overwriting contents)
 *   * read_string_from_file(filename,str,size): read a string of at most <size> (incl.
 *     NULL character) from file <filename> and store it into <str>. Note that <str>
 *     needs to be large enough to hold that string.
 *  - You NEED to test your code. The autograder we provide does not contain the
 *    full set of tests that we will run on your code. See "Step 5" in the project spec.
 */

/* beargit init
 *
 * - Create .beargit directory
 * - Create empty .beargit/.index file
 * - Create .beargit/.prev file containing 0..0 commit id
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_init(void) {
  fs_mkdir(".beargit");

  FILE* findex = fopen(".beargit/.index", "w");
  fclose(findex);

  FILE* fbranches = fopen(".beargit/.branches", "w");
  fprintf(fbranches, "%s\n", "master");
  fclose(fbranches);

  write_string_to_file(".beargit/.prev", "0000000000000000000000000000000000000000");
  write_string_to_file(".beargit/.current_branch", "master");

  return 0;
}



/* beargit add <filename>
 *
 * - Append filename to list in .beargit/.index if it isn't in there yet
 *
 * Possible errors (to stderr):
 * >> ERROR:  File <filename> has already been added.
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_add(const char* filename) {
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) {
      fprintf(stderr, "ERROR:  File %s has already been added.\n", filename);
      fclose(findex);
      fclose(fnewindex);
      fs_rm(".beargit/.newindex");
      return 3;
    }

    fprintf(fnewindex, "%s\n", line);
  }

  fprintf(fnewindex, "%s\n", filename);
  fclose(findex);
  fclose(fnewindex);

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}

/* beargit status
 *
 * See "Step 1" @hive1.cs.berkeley.eduin the project spec.
 *
 */

int beargit_status() {
  printf("Tracked files:\n");
  printf("\n");
  int count = 0;
  FILE* findex = fopen(".beargit/.index", "r");
  if (findex != NULL) {
    char line [FILENAME_SIZE];
    while (fgets(line, sizeof(line), findex) != NULL) {
      printf("%s", line);
      count++;
    }
    fclose (findex);
    printf("\nThere are ");
    printf("%d", count);
    printf(" files total.\n");
  }
  return 0;
}

/* beargit rm <filename>
 *
 * See "Step 2" in the project spec.
 *
 */

int beargit_rm(const char* filename) {
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");
  
  int count = 0;
  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    if (strcmp(line, filename) != 0) {
      fprintf(fnewindex, "%s\n", line);
    } else {
      count++;
    }
  }
  if (count == 0) {
    fprintf(stderr, "ERROR:  File %s not tracked.\n", filename);
    return 1;
  }

  fclose(findex);
  fclose(fnewindex);

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}

/* beargit commit -m <msg>
 *
 * See "Step 3" in the project spec.
 *
 */

const char* go_bears = "THIS IS BEAR TERRITORY!";

//if message is ok then return 1 if it is not return 0
int is_commit_msg_ok(const char* msg) {
  char *c = (char*)go_bears;
  char *b = (char*)msg;
  int i = 0;
  int j = 0;
  while (b[i] != '\0') {
    if (c[j+1] == '\0') {
      return 1;
    }
    else if (b[i] != c[j]) {
      j = 0;
      i++;
    }
    else if (b[i] == c[j]) {
      i++;
      j++;
    }
  }
  return 0;
}

/* Use next_commit_id to fill in the rest of the commit ID.
 *
 * Hints:
 * You will need a destination string buffer to hold your next_commit_id, 
 * before you copy it back to commit_id
 * You will need to use a function we have provided for you.
 */

//https://blog.udemy.com/c-string-to-int/ how to convert string to int
//http://faq.cprogramming.com/cgi-bin/smartfaq.cgi?answer=1043808026&id=1043284385 
// ^---- how to convert int to string
void next_commit_id(char* commit_id) {
  char hash [COMMIT_ID_SIZE];
  char actual[COMMIT_ID_SIZE * 2];
  FILE* curr = fopen(".beargit/.current_branch", "r");
  char line1[BRANCHNAME_SIZE];
  if (fgets(line1, sizeof(line1), curr) != NULL) {
  }
  fclose (curr);
  strtok(line1, "\n");
  strcpy(actual, line1); //creates nonhashed id
  strcat(actual, commit_id);
  cryptohash(actual, hash);

  char *beargit = ".beargit/";
  char newdir[BRANCHNAME_SIZE + COMMIT_ID_SIZE];
  strcpy(newdir, beargit);
  strcat(newdir, hash);
  fs_mkdir(newdir); //.beargit/<newid>

  char *prev = "/.prev"; //.beargit/<newid>/.prev
  char newprev[BRANCHNAME_SIZE + COMMIT_ID_SIZE];
  strcpy(newprev, newdir);
  strcat(newprev, prev);
  write_string_to_file(newprev, commit_id);

  char *index = "/.index";
  char newin[BRANCHNAME_SIZE + COMMIT_ID_SIZE];
  strcpy(newin, newdir);
  strcat(newin, index); //.beargit/<newid>/.index

  //read index and copy all files into the new directory
  FILE* findex = fopen(".beargit/.index", "r");
  if (findex != NULL) {
    char linee [FILENAME_SIZE];
    while (fgets(linee, sizeof(linee), findex) != NULL) {
      strtok(linee, "\n");
      char newfile[BRANCHNAME_SIZE + COMMIT_ID_SIZE];
      char oldfile[BRANCHNAME_SIZE + COMMIT_ID_SIZE];
      strcpy(newfile, newdir);
      strcat(newfile, "/");
      strcat(newfile, linee);
      strcpy(oldfile, linee);
      fs_cp(oldfile, newfile);
    }
  }
    
  fs_cp(".beargit/.index", newin);
  write_string_to_file(".beargit/.prev", hash);
  fclose(findex);
}

int beargit_commit(const char* msg) {
  if (!is_commit_msg_ok(msg)) {
    fprintf(stderr, "ERROR:  Message must contain \"%s\"\n", go_bears);
    return 1;
  }
  //branch is not HEAD
  FILE* curr = fopen(".beargit/.current_branch", "r");
  char branch[BRANCHNAME_SIZE];
  fgets(branch, sizeof(branch), curr);
  strtok(branch, "\n");
  if ((strcmp(branch, "")) == 0) {
    fprintf(stderr, "ERROR:  Need to be on HEAD of a branch to commit.\n");
    return 0;
    }
  fclose (curr);
  //write msg into .beargit/<newid>/.msg
  char commit_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
  next_commit_id(commit_id);
  char *m = "/.msg"; //.beargit/<newid>/.msg
  char *b = ".beargit/";
  char newmsg[BRANCHNAME_SIZE + COMMIT_ID_SIZE];
  strcpy(newmsg, b);
  char line [FILENAME_SIZE];
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
  strcat(newmsg, commit_id);
  strcat(newmsg, m);
  write_string_to_file(newmsg, msg);
  return 0;
}

/* beargit log
 *
 * See "Step 4" in the project spec.
 *
 */

int beargit_log(int limit) {

  /** Find the most recent commit. **/
  FILE *latest = fopen(".beargit/.prev", "r");
  char line[COMMIT_ID_SIZE];
  char hash[COMMIT_ID_SIZE];
  fgets(line, sizeof(line), latest);
  strtok(line, "\n");
  fclose(latest);

  /** Check if there exists a commit. **/
  int count = 0;
  for (int i = 0; line[i] != '\0'; i++) {
    if (line[i] != '0') {
      count++;
    }
  }
  if (count == 0) {
    fprintf(stderr, "ERROR:  There are no commits.\n");
    return 1;
  }
  /** Print commit id and message for each commit up to limit. **/
  int lim = 0;
  while (lim != limit) {
    
    /** Retrieve message from commit. **/
    char id[COMMIT_ID_SIZE + 50];
    strcpy(id, ".beargit/");
    strcat(id, line);
    strcat(id, "/.msg");
    FILE *msg = fopen(id, "r");
    char message[MSG_SIZE];
    fgets(message, sizeof(message), msg);
    /** Print commit id and message for this commit. **/
    printf("commit ");
    printf("%s\n", line);
    printf("   ");
    printf("%s\n\n", message);
    /** Clear memory for id and message arrays for next iteration. **/
    memset(id, 0, sizeof(id));
    memset(message, 0, sizeof(message));
    fclose(msg);
    /** Retrieve the next previous commit id. **/
    char previous[COMMIT_ID_SIZE + 50];
    strcpy(previous, ".beargit/");
    strcat(previous, line);
    strcat(previous, "/.prev");
    FILE *prev = fopen(previous, "r");
    memset(line, 0, sizeof(line)); //Clear memory for array containing commit id.
    fgets(line, sizeof(line), prev);
    fclose(prev);
    /** Check if there exists a previous commit. **/
    int count = 0;
    strtok(line, "\n");
    // printf("%s\n", line);
    for (int i = 0; line[i] != '\0'; i++) {
      if (line[i] != '0') {
        count++;
      }
    }
    if (count == 0) {
      return 0;
    }
    // cryptohash(line, hash);
    count = 0; //restart counter for next iteration
    lim++;
  }
  return 0;
}


// This helper function returns the branch number for a specific branch, or
// returns -1 if the branch does not exist.
int get_branch_number(const char* branch_name) {
  FILE* fbranches = fopen(".beargit/.branches", "r");

  int branch_index = -1;
  int counter = 0;
  char line[BRANCHNAME_SIZE];
  while(fgets(line, sizeof(line), fbranches)) {
    strtok(line, "\n");
    if (strcmp(line, branch_name) == 0) {
      branch_index = counter;
    }
    counter++;
  }

  fclose(fbranches);

  return branch_index;
}

/* beargit branch
 *
 * See "Step 5" in the project spec.
 *
 */

int beargit_branch() {
  FILE* fcurr = fopen(".beargit/.current_branch", "r");
  char curr [BRANCHNAME_SIZE];
  char line [BRANCHNAME_SIZE];
  char s1 [BRANCHNAME_SIZE];
  char s2 [BRANCHNAME_SIZE];
  if (fcurr != NULL) {
    fgets(curr, sizeof(curr), fcurr);
  }
  fclose(fcurr);

  FILE* fbranch = fopen(".beargit/.branches", "r");
  if (fbranch != NULL) {
    while (fgets(line, sizeof(line), fbranch) != NULL) {
      char blank[BRANCHNAME_SIZE];
      char new[BRANCHNAME_SIZE];
      int c = 0, d = 0, c1 = 0, d1 = 0;
      strtok(curr, "\n");
      strtok(line, "\n");
      int g = strcmp(curr, line);
      if (g == 0) {
        printf("%s", "*  ");
        printf("%s\n", line);
      } else {
        printf("%s", "   ");
        printf("%s\n", line);
      }
    }
    fclose (fbranch);
  }
  return 0;
}

/* beargit checkout
 *
 * See "Step 6" in the project spec.
 *
 */

int checkout_commit(const char* commit_id) {

  /** Delete all the files in the current index file. **/
  FILE *findex = fopen(".beargit/.index", "r");
  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    fs_rm(line);
  }
  fclose(findex);

  /** Write the ID of the checkout commit into .prev. **/
  write_string_to_file(".beargit/.prev", commit_id);

  /** Check to see if the checkout commit_id is 0. **/
  int count = 0;
  for (int i = 0; commit_id[i] != '\0'; i++) {
    if (commit_id[i] != '0') {
      count++;
    }
  }
  if (count == 0) {
    FILE *empty = fopen(".beargit/.index", "w");
    fclose(empty);
    return 0;
  }
  
  /** Copy the index from the checkout commit to the beargit index file. **/
  char file[COMMIT_ID_SIZE + 20] = ".beargit/";
  strcat(file, commit_id);
  strcat(file, "/.index");
  fs_cp(file, ".beargit/.index");

  /** Copy all the checkout commit's tracked files into the current directory. **/
  FILE *new_findex = fopen(file, "r");
  char new_line[FILENAME_SIZE];
  while(fgets(new_line, sizeof(new_line), new_findex)) {
    strtok(new_line, "\n");
    char new_file[FILENAME_SIZE + 20] = ".beargit/";
    strcat(new_file, commit_id);
    strcat(new_file, "/");
    strcat(new_file, new_line);
    char temp[FILENAME_SIZE] = "./";
    strcat(temp, new_line);
    fs_cp(new_file, temp);
    memset(new_file, 0, sizeof(new_file));
    memset(temp, 0, sizeof(temp));
  }
  fclose(new_findex);

  return 0;
}

int is_it_a_commit_id(const char* commit_id) {
  char file[COMMIT_ID_SIZE + 20] = ".beargit/";
  strcat(file, commit_id);
  if (fs_check_dir_exists(file)) {
    return 1;
  }
  else {
    return 0;
  }
}

int beargit_checkout(const char* arg, int new_branch) {
  // Get the current branch

  char current_branch[BRANCHNAME_SIZE];
  read_string_from_file(".beargit/.current_branch", current_branch, BRANCHNAME_SIZE);

   // If not detached, leave the current branch by storing the current HEAD into that branch's file...
    if (strlen(current_branch)) {
    char current_branch_file[BRANCHNAME_SIZE+50];
    sprintf(current_branch_file, ".beargit/.branch_%s", current_branch);
    fs_cp(".beargit/.prev", current_branch_file);
  }

  // Check whether the argument is a commit ID. If yes, we just change to detached mode
  // without actually having to change into any other branch.
  if (is_it_a_commit_id(arg)) {
    char commit_dir[FILENAME_SIZE] = ".beargit/";
    strcat(commit_dir, arg);

    // ...and setting the current branch to none (i.e., detached).
    write_string_to_file(".beargit/.current_branch", "");

    return checkout_commit(arg);
  }
  
  // Read branches file (giving us the HEAD commit id for that branch).
  int branch_exists = (get_branch_number(arg) >= 0);

  // Check for errors.
  if (branch_exists && new_branch) {
    fprintf(stderr, "ERROR:  A branch named %s already exists.\n", arg);
    return 1;
  } else if (!branch_exists && !new_branch) {
    fprintf(stderr, "ERROR:  No branch or commit %s exists.\n", arg);
    return 1;
  }
  
  // Just a better name, since we now know the argument is a branch name.
  const char* branch_name = arg;

  // File for the branch we are changing into.
  char branch_file[BRANCHNAME_SIZE + 20] = ".beargit/.branch_";
  strcat(branch_file, branch_name);
  
  // Update the branch file if new branch is created (now it can't go wrong anymore)
  if (new_branch) {
    FILE* fbranches = fopen(".beargit/.branches", "a");
    fprintf(fbranches, "%s\n", branch_name);
    fclose(fbranches);
    fs_cp(".beargit/.prev", branch_file);
  }

  write_string_to_file(".beargit/.current_branch", branch_name);

  // Read the head commit ID of this branch.
  char branch_head_commit_id[COMMIT_ID_SIZE];
  read_string_from_file(branch_file, branch_head_commit_id, COMMIT_ID_SIZE);

  // Check out the actual commit.
  return checkout_commit(branch_head_commit_id);
}

/* beargit reset
 *
 * See "Step 7" in the project spec.
 *
 */

int beargit_reset(const char* commit_id, const char* filename) {
  if (!is_it_a_commit_id(commit_id)) {
      fprintf(stderr, "ERROR:  Commit %s does not exist.\n", commit_id);
      return 1;
  }

  // Check if the file is in the commit directory
  char dir[COMMIT_ID_SIZE + FILENAME_SIZE + 20];
  strcpy(dir, ".beargit/");
  strcat(dir, commit_id);
  strcat(dir, "/");
  strcat(dir, filename);
  int h = 0;
  char full[COMMIT_ID_SIZE + FILENAME_SIZE + 20];
  strcpy(full, ".beargit/");
  strcat(full, commit_id);
  strcat(full, "/.index");
  FILE* cindex = fopen(full, "r");
  if (cindex != NULL) {
    char line [FILENAME_SIZE];
    while ((fgets(line, sizeof(line), cindex) != NULL) || (h != 1)) {
      strtok(line, "\n");
      if (strcmp(line, filename) == 0) {
        h = 1;
      }
    }
    fclose (cindex);
  }
  if (h == 0) {
    fprintf(stderr, "ERROR:  %s is not in the index of commit %s.\n", filename, commit_id);
    return 1;
  }
  char fn[FILENAME_SIZE];
  strcat(fn, filename);
  // Copy the file to the current working directory
  fs_cp(dir, filename);


  // Add the file if it wasn't already there
  FILE* findex1 = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  char line2[FILENAME_SIZE];
  while(fgets(line2, sizeof(line2), findex1)) {
    strtok(line2, "\n");
    if (strcmp(line2, filename) == 0) {
      fclose(findex1);
      fclose(fnewindex);
      fs_rm(".beargit/.newindex");
      return 0;
    }

    fprintf(fnewindex, "%s\n", line2);
  }

  fprintf(fnewindex, "%s\n", filename);
  fclose(findex1);
  fclose(fnewindex);

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}


/* beargit merge
 *
 * See "Step 8" in the project spec.
 *
 */

int beargit_merge(const char* arg) {
  // Get the commit_id or throw an error
  char commit_id[COMMIT_ID_SIZE];
  if (!is_it_a_commit_id(arg)) {
      if (get_branch_number(arg) == -1) {
            fprintf(stderr, "ERROR:  No branch or commit %s exists.\n", arg);
            return 1;
      }
      char branch_file[FILENAME_SIZE];
      snprintf(branch_file, FILENAME_SIZE, ".beargit/.branch_%s", arg);
      read_string_from_file(branch_file, commit_id, COMMIT_ID_SIZE);
  } else {
      snprintf(commit_id, COMMIT_ID_SIZE, "%s", arg);
  }

  // Iterate through each line of the commit_id index and determine how you
  // should copy the index file over
  char path[COMMIT_ID_SIZE + 20] = ".beargit/";
  strcat(path, commit_id);
  strcat(path, "/");
  strcat(path, ".index");
  FILE *merge_index = fopen(path, "r");
  char line[FILENAME_SIZE];
  int count;
  while(fgets(line, sizeof(line), merge_index)) {
    strtok(line, "\n");
    char new_path[COMMIT_ID_SIZE + FILENAME_SIZE + 20] = ".beargit/";
    strcat(new_path, commit_id);
    strcat(new_path, "/");
    strcat(new_path, line);

    FILE *findex = fopen(".beargit/.index", "r");
    FILE *fnewindex = fopen(".beargit/.newindex", "w");

    char cline[FILENAME_SIZE];
    count = 0;
    while(fgets(cline, sizeof(cline), findex)) {
      strtok(cline, "\n");
      fprintf(fnewindex, "%s\n", cline);
      if (strcmp(cline, line) == 0) {
        count++;
      }
    }
    if (count != 0) {
      printf("%s", line);
      printf("%s\n", " conflicted copy created");
      strcat(line, ".");
      strcat(line, commit_id);
      fs_cp(new_path, line);
    }
    else {
      fs_cp(new_path, line);
      fprintf(fnewindex, "%s\n", line);
      printf("%s", line);
      printf("%s\n", " added");
    }
    fclose(findex);
    fclose(fnewindex);

    fs_mv(".beargit/.newindex", ".beargit/.index");
    
    memset(new_path, 0, sizeof(new_path));
  }
  fclose(merge_index);

  return 0;
}
