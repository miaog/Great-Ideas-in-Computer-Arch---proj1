#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <CUnit/Basic.h>
#include "beargit.h"
#include "util.h"

/* printf/fprintf calls in this tester will NOT go to file. */

#undef printf
#undef fprintf

/* The suite initialization function.
 * You'll probably want to delete any leftover files in .beargit from previous
 * tests, along with the .beargit directory itself.
 *
 * You'll most likely be able to share this across suites.
 */
int init_suite(void)
{
    // preps to run tests by deleting the .beargit directory if it exists
    fs_force_rm_beargit_dir();
    unlink("TEST_STDOUT");
    unlink("TEST_STDERR");
    return 0;
}

/* You can also delete leftover files after a test suite runs, but there's
 * no need to duplicate code between this and init_suite 
 */
int clean_suite(void)
{
    return 0;
}

/* Simple test of fread().
 * Reads the data previously written by testFPRINTF()
 * and checks whether the expected characters are present.
 * Must be run after testFPRINTF().
 */
void simple_sample_test(void)
{
    // This is a very basic test. Your tests should likely do more than this.
    // We suggest checking the outputs of printfs/fprintfs to both stdout
    // and stderr. To make this convenient for you, the tester replaces
    // printf and fprintf with copies that write data to a file for you
    // to access. To access all output written to stdout, you can read 
    // from the "TEST_STDOUT" file. To access all output written to stderr,
    // you can read from the "TEST_STDERR" file.
    int retval;
    retval = beargit_init();
    CU_ASSERT(0==retval);
    retval = beargit_add("asdf.txt");
    CU_ASSERT(0==retval);
}

struct commit {
  char msg[MSG_SIZE];
  struct commit* next;
};


void free_commit_list(struct commit** commit_list) {
  if (*commit_list) {
    free_commit_list(&((*commit_list)->next));
    free(*commit_list);
  }

  *commit_list = NULL;
}

void run_commit(struct commit** commit_list, const char* msg) {
    int retval = beargit_commit(msg);
    CU_ASSERT(0==retval);

    struct commit* new_commit = (struct commit*)malloc(sizeof(struct commit));
    new_commit->next = *commit_list;
    strcpy(new_commit->msg, msg);
    *commit_list = new_commit;
}

void simple_log_test(void)
{
    struct commit* commit_list = NULL;
    int retval;
    retval = beargit_init();
    CU_ASSERT(0==retval);
    FILE* asdf = fopen("asdf.txt", "w");
    fclose(asdf);
    retval = beargit_add("asdf.txt");
    CU_ASSERT(0==retval);
    run_commit(&commit_list, "THIS IS BEAR TERRITORY!2");
    run_commit(&commit_list, "THIS IS BEAR TERRITORY!3");

    retval = beargit_log(10);
    CU_ASSERT(0==retval);

    struct commit* cur_commit = commit_list;

    const int LINE_SIZE = 512;
    char line[LINE_SIZE];

    FILE* fstdout = fopen("TEST_STDOUT", "r");
    CU_ASSERT_PTR_NOT_NULL(fstdout);

    while (cur_commit != NULL) {
      char refline[LINE_SIZE];

      // First line is commit -- don't check the ID.
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT(!strncmp(line,"commit", strlen("commit")));

      // Second line is msg
      sprintf(refline, "   %s\n", cur_commit->msg);
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line, refline);

      // Third line is empty
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT(!strcmp(line,"\n"));

      cur_commit = cur_commit->next;
    }

    CU_ASSERT_PTR_NULL(fgets(line, LINE_SIZE, fstdout));

    // It's the end of output
    CU_ASSERT(feof(fstdout));
    fclose(fstdout);

    free_commit_list(&commit_list);
}

void status_test(void) {
  /** This test is designed to test the print values of beargit status.
      There are 4 different parts to this test to ensure there are no errors.
      The parts and what they test are as follows:
          1) Check that there are 0 files listed if you did not add any files.
          2) Check that after you add a file, there should be 1 file listed.
          3) Check that after you commit your changes and add another file,
              there are a total of 2 files listed.
          4) Check that if you remove the first file, then that file should 
              no longer be tracked and so all you will see is the second
              file you added is tracked so there are a total of 1 tracked files.
          5) Check that if you add a lot of files then you will get all of those
              files tracked 
      Each one of these parts will be separated by a line of *'s like so:
      *************************************************************************
      **/
    const int LINE_SIZE = 512;
    char line[LINE_SIZE];
    int retval;
    retval = beargit_init();
    CU_ASSERT(0==retval);
    //check that there are 0 tracked files listed
    retval = beargit_status();
    CU_ASSERT(0==retval);

    FILE* fstdout = fopen("TEST_STDOUT", "r");
    CU_ASSERT_PTR_NOT_NULL(fstdout);
    int x = 1;
    while (x != 0) {
      //first line is -- Tracked files:
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line, "Tracked files:\n");

      //second line is empty
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line, "\n");

      //third line is empty
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line, "\n");

      //fourth line is -- There are 0 files total.
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line, "There are 0 files total.\n");
      x = 0;
    }
    //line should be NULL since no more output
    CU_ASSERT_PTR_NULL(fgets(line, LINE_SIZE, fstdout));

    /************************************************************************/
    //Testing status of 1 added file
    char line1[LINE_SIZE];
    FILE* asdf = fopen("asdf.txt", "w");
    fclose(asdf);
    retval = beargit_add("asdf.txt");
    CU_ASSERT(0==retval);
    retval = beargit_status();
    CU_ASSERT(0==retval);
    CU_ASSERT_PTR_NOT_NULL(fstdout);
    x = 1;
    while (x != 0) {
      //first line is -- Tracked files:
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "Tracked files:\n");

      //second line is empty
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "\n");

      //third line is asdf.txt
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "asdf.txt\n");

      //fourth line is empty
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "\n");

      //fifth line is -- There are 1 files total.
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "There are 1 files total.\n");
      x = 0;
    }

    /************************************************************************/
    //Test that previously tracked file is still there after commit
    //and the newly added file is there too.
    struct commit* commit_list = NULL;
    run_commit(&commit_list, "THIS IS BEAR TERRITORY!1");
    char line2[LINE_SIZE];
    CU_ASSERT(0==retval);
    FILE* b = fopen("b.txt", "w");
    fclose(b);
    retval = beargit_add("b.txt");
    CU_ASSERT(0==retval);
    retval = beargit_status();
    CU_ASSERT(0==retval);
    CU_ASSERT_PTR_NOT_NULL(fstdout);
    x = 1;
    while (x != 0) {
      //first line is -- Tracked files:
      CU_ASSERT_PTR_NOT_NULL(fgets(line2, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line2, "Tracked files:\n");

      //second line is empty
      CU_ASSERT_PTR_NOT_NULL(fgets(line2, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line2, "\n");

      //third line is asdf.txt
      CU_ASSERT_PTR_NOT_NULL(fgets(line2, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line2, "asdf.txt\n");

      //fourth line is b.txt
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "b.txt\n");

      //fifth line is empty
      CU_ASSERT_PTR_NOT_NULL(fgets(line2, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line2, "\n");

      //sixth line is -- There are 2 files total.
      CU_ASSERT_PTR_NOT_NULL(fgets(line2, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line2, "There are 2 files total.\n");
      x = 0;
    }

    /************************************************************************/
    //Test that after you remove a file, it is no longer tracked
    retval = beargit_rm("asdf.txt");
    //remove asdf.txt so it should not be tracked anymore
    CU_ASSERT(0==retval);
    retval = beargit_status();
    CU_ASSERT(0==retval);
    CU_ASSERT_PTR_NOT_NULL(fstdout);
    x = 1;
    while (x != 0) {
      //first line is -- Tracked files:
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "Tracked files:\n");

      //second line is empty
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "\n");

      //third line is b.txt
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "b.txt\n");

      //fourth line is empty
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "\n");

      //fifth line is -- There are 1 files total.
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "There are 1 files total.\n");
      x = 0;
    }

    /************************************************************************/
    //Test that adding a lot of files will print out the correct outcome.
    FILE* c = fopen("c.txt", "w");
    fclose(c);
    FILE* d = fopen("d.txt", "w");
    fclose(d);
    FILE* e = fopen("e.txt", "w");
    fclose(e);
    FILE* f = fopen("f.txt", "w");
    fclose(f);
    FILE* g = fopen("g.txt", "w");
    fclose(g);
    FILE* h = fopen("h.txt", "w");
    fclose(h);
    FILE* i = fopen("i.txt", "w");
    fclose(i);
    FILE* j = fopen("j.txt", "w");
    fclose(j);
    retval = beargit_add("c.txt");
    retval = beargit_add("d.txt");
    retval = beargit_add("e.txt");
    retval = beargit_add("f.txt");
    retval = beargit_add("g.txt");
    retval = beargit_add("h.txt");
    retval = beargit_add("i.txt");
    retval = beargit_add("j.txt");
    CU_ASSERT(0==retval);
    retval = beargit_status();
    CU_ASSERT(0==retval);
    CU_ASSERT_PTR_NOT_NULL(fstdout);
    x = 1;
    while (x != 0) {
      //first line is -- Tracked files:
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "Tracked files:\n");

      //second line is empty
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "\n");

      //third line is b.txt
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "b.txt\n");

      //fourth line is c.txt
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "c.txt\n");

      //fourth line is d.txt
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "d.txt\n");

      //fourth line is e.txt
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "e.txt\n");

      //fourth line is f.txt
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "f.txt\n");

      //fourth line is g.txt
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "g.txt\n");

      //fourth line is h.txt
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "h.txt\n");

      //fourth line is i.txt
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "i.txt\n");

      //fourth line is j.txt
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "j.txt\n");

      //fifth line is empty
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "\n");

      //sixth line is -- There are 8 files total.
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "There are 9 files total.\n");
      x = 0;
    }
    fclose(fstdout);

}

void branch_test(void) {
    /** This test is designed to test the print values of beargit_branch. 
        There are 3 different tests in this test suite:
          1) Tests that beargit_branch prints the current branch "master" 
             after beargit_init is called.  Makes sure that the * is before 
             "master".
          2) Tests that beargit_branch prints the new branch after
             beargit_checkout is called to create a new branch called "new".
             Tests whether the * is placed before "new" to indicate that
             the current branch is "new".
          3) Tests that beargit_branch prints both branches with the current
             branch now as "master" after beargit_checkout is called on "master".
             Makes sure that * is now placed in front of "master" as opposed to "new", 
             which used to be the current branch.
          4) Tests that once a commit is made and that commit is checked out using
             beargit_checkout on the commit id, there is no * next to either branch 
             because the commit becomes detached and does not belong to a branch.
        The tests will be separated by a line of *'s like so:
        /************************************************************************
    **/
    const int LINE_SIZE = 512;
    char line[LINE_SIZE];
    
    int retval;
    retval = beargit_init();
    CU_ASSERT(0==retval);
    //check that there is only the master branch and we are currently on it
    retval = beargit_branch();
    CU_ASSERT(0==retval);

    FILE* fstdout = fopen("TEST_STDOUT", "r");
    CU_ASSERT_PTR_NOT_NULL(fstdout);
    int i = 1;
    while (i != 0) {
      //first line is -- *  master:
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line, "*  master\n");

      i = 0;
    }
    //line should be NULL since no more output
    CU_ASSERT_PTR_NULL(fgets(line, LINE_SIZE, fstdout));

    /************************************************************************/
    //Testing branch with a new branch
    char line1[LINE_SIZE];
    retval = beargit_checkout("new", 1);
    CU_ASSERT(0==retval);
    retval = beargit_branch();
    CU_ASSERT(0==retval);
    CU_ASSERT_PTR_NOT_NULL(fstdout);
    i = 1;
    while (i != 0) {
      //first line is -- master:
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "   master\n");

      //second line is -- *  new:
      CU_ASSERT_PTR_NOT_NULL(fgets(line1, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line1, "*  new\n");

      i = 0;
    }

    /************************************************************************/
    //Testing switching back to master branch as current branch
    char line2[LINE_SIZE];
    retval = beargit_checkout("master", 0);
    CU_ASSERT(0==retval);
    retval = beargit_branch();
    CU_ASSERT(0==retval);
    CU_ASSERT_PTR_NOT_NULL(fstdout);
    i = 1;
    while (i != 0) {
      //first line is -- *  master:
      CU_ASSERT_PTR_NOT_NULL(fgets(line2, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line2, "*  master\n");

      //second line is --    new:
      CU_ASSERT_PTR_NOT_NULL(fgets(line2, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line2, "   new\n");

      i = 0;
    }

    /************************************************************************/
    //Make two commits and then checkout the first commit that was made
    struct commit* commit_list = NULL;
    FILE* asdf = fopen("asdf.txt", "w");
    fclose(asdf);
    retval = beargit_add("asdf.txt");
    CU_ASSERT(0==retval);
    run_commit(&commit_list, "THIS IS BEAR TERRITORY!2");

    char linee[LINE_SIZE];
    retval = beargit_log(10);
    CU_ASSERT(0==retval);
    CU_ASSERT_PTR_NOT_NULL(fgets(linee, LINE_SIZE, fstdout)); //commit & commit ID in log
    CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout)); //message for log
    CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout)); //blank line for log
    strtok(linee, "\n");
    char *b = linee + 7;

    retval = beargit_checkout(b, 0);
    CU_ASSERT(0==retval);
    retval = beargit_branch();
    CU_ASSERT(0==retval);
    CU_ASSERT_PTR_NOT_NULL(fstdout);
    i = 1;
    while (i != 0) {
      //first line is --    master:
      CU_ASSERT_PTR_NOT_NULL(fgets(linee, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(linee, "   master\n");

      //second line is --    new:
      CU_ASSERT_PTR_NOT_NULL(fgets(linee, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(linee, "   new\n");

      i = 0;
    }
    fclose(fstdout);

}

/* The main() function for setting up and running the tests.
 * Returns a CUE_SUCCESS on successful running, another
 * CUnit error code on failure.
 */
int cunittester()
{
   CU_pSuite pSuite = NULL;
   CU_pSuite pSuite2 = NULL;
   CU_pSuite pSuite3 = NULL;
   CU_pSuite pSuite4 = NULL;

   /* initialize the CUnit test registry */
   if (CUE_SUCCESS != CU_initialize_registry())
      return CU_get_error();

   /* add a suite to the registry */
   pSuite = CU_add_suite("Suite_1", init_suite, clean_suite);
   if (NULL == pSuite) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* Add tests to the Suite #1 */
   if (NULL == CU_add_test(pSuite, "Simple Test #1", simple_sample_test))
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   pSuite2 = CU_add_suite("Suite_2", init_suite, clean_suite);
   if (NULL == pSuite2) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* Add tests to the Suite #2 */
   if (NULL == CU_add_test(pSuite2, "Log output test", simple_log_test))
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   pSuite3 = CU_add_suite("Suite_3", init_suite, clean_suite);
   if (NULL == pSuite3) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   if (NULL == CU_add_test(pSuite3, "Test Status", status_test))
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   pSuite4 = CU_add_suite("Suite_4", init_suite, clean_suite);
   if (NULL == pSuite4) {
      CU_cleanup_registry();
      return CU_get_error();
   }
   if (NULL == CU_add_test(pSuite4, "Test Branch", branch_test))
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* Run all tests using the CUnit Basic interface */
   CU_basic_set_mode(CU_BRM_VERBOSE);
   CU_basic_run_tests();
   CU_cleanup_registry();
   return CU_get_error();
}