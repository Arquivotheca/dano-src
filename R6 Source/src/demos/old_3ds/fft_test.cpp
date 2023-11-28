/*
 * Copyright (c) 1997 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to use, copy, modify, and distribute the Software without
 * restriction, provided the Software, including any modified copies made
 * under this license, is not distributed for a fee, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE MASSACHUSETTS INSTITUTE OF TECHNOLOGY BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * Except as contained in this notice, the name of the Massachusetts
 * Institute of Technology shall not be used in advertising or otherwise
 * to promote the sale, use or other dealings in this Software without
 * prior written authorization from the Massachusetts Institute of
 * Technology.
 *  
 */

/*
 * fftw_test.c : test program
 */

/* $Id: fftw_test.c,v 1.27 1997/09/04 19:14:21 fftw Exp $ */
#include <fftw.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#if defined(macintosh) || defined(__WIN32__)
#undef HAVE_GETOPT
#else
#define HAVE_GETOPT
#endif
#undef HAVE_GETOPT
#ifdef HAVE_GETOPT
#include <unistd.h>
#endif

/********************
 *   macrology: 
 ********************/
#ifndef TRUE
#define TRUE 42
#endif
#ifndef FALSE
#define FALSE (!TRUE)
#endif

#define CHECK(condition, complaint)      \
    if (!(condition)) {                  \
        fprintf(stderr, "FATAL ERROR: %s\n", complaint);      \
        exit(1);                         \
    }

#define WHEN_VERBOSE(x)  if (verbose) x

#define DRAND() (((double) rand()) / 31415)

/*******************
 * global variables
 *******************/
int verbose;
int wisdom_flags;

/*******************
 * procedures
 *******************/

/*************************************************
 * Speed tests
 *************************************************/
void test_speed_aux(int n, fftw_direction dir, int flags)
{
     FFTW_COMPLEX *in, *out;
     fftw_plan plan;
     int i, iter;
     double t;
     fftw_time begin, end;

     iter = 1;

     in = (FFTW_COMPLEX *)fftw_malloc(n * sizeof(FFTW_COMPLEX));
     out = (FFTW_COMPLEX *)fftw_malloc(n * sizeof(FFTW_COMPLEX));

     begin = fftw_get_time();
     plan = fftw_create_plan(n, dir, FFTW_MEASURE | flags | wisdom_flags);
     end = fftw_get_time();
     CHECK(plan != NULL, "can't create plan");

     t = fftw_time_to_sec(fftw_time_diff(end,begin));
     WHEN_VERBOSE(printf("time for planner: %f s\n", t));

     WHEN_VERBOSE(fftw_print_plan(plan));

     do {
	  begin = fftw_get_time();
	  for (i = 0; i < iter; ++i) {
	       int j;
	       
	       /* reset the array to known values */
	       for (j = 0; j < n; ++j) {
		    c_re(in[j]) = 1.0;
		    c_im(in[j]) = 32.432;
	       }
	       
	       fftw(plan, 1, in, 1, 0, out, 1, 0);
	  }
	  end = fftw_get_time();
	  t = fftw_time_to_sec(fftw_time_diff(end,begin));
	  if (t < 1.0)
	       iter *= 2;
     } while (t < 1.0);

     fftw_destroy_plan(plan);

     WHEN_VERBOSE(printf("Number of iterations = %d\n", iter));
     WHEN_VERBOSE(printf("time for iterations: %f s\n", t));

     t = t / (double) iter;

     printf("time for one fft: %e s (%e s/point)\n", t, t / n);
     printf("normalized time t / (n log n) = %e\n",
	    t / ((double) n * log((double) n)));
     printf("\"mflops\" = 5 (n log2 n) / (t in microseconds) = %f\n",
	    5.0 * n * log((double) n) / log(2.0) / (t * 1.0e6));

     fftw_free(in);
     fftw_free(out);

     printf("\n");
}

void test_speed(int n)
{
     printf("SPEED TEST: n = %d, FFTW_FORWARD, out of place\n", n);
     test_speed_aux(n, FFTW_FORWARD, 0);

     printf("SPEED TEST: n = %d, FFTW_FORWARD, in place\n", n);
     test_speed_aux(n, FFTW_FORWARD, FFTW_IN_PLACE);

     printf("SPEED TEST: n = %d, FFTW_BACKWARD, out of place\n", n);
     test_speed_aux(n, FFTW_BACKWARD, 0);

     printf("SPEED TEST: n = %d, FFTW_BACKWARD, in place\n", n);
     test_speed_aux(n, FFTW_BACKWARD, FFTW_IN_PLACE);
}

/*************************************************
 * correctness tests
 *************************************************/     
void test_out_of_place(int n, int istride, int ostride, 
		       int howmany, fftw_direction dir)
{
     FFTW_COMPLEX *in1, *in2, *out1, *out2;
     fftw_plan plan;
     int i, j;
     double error, a;

     in1 = (FFTW_COMPLEX *)fftw_malloc(istride * n * sizeof(FFTW_COMPLEX) * howmany);
     in2 = (FFTW_COMPLEX *)fftw_malloc(n * sizeof(FFTW_COMPLEX) * howmany);
     out1 = (FFTW_COMPLEX *)fftw_malloc(ostride * n * sizeof(FFTW_COMPLEX) * howmany);
     out2 = (FFTW_COMPLEX *)fftw_malloc(n * sizeof(FFTW_COMPLEX) * howmany);

     /* generate random inputs */
     for (i = 0; i < n * howmany; ++i) {
	  c_re(in1[i * istride]) = c_re(in2[i]) = DRAND();
	  c_im(in1[i * istride]) = c_im(in2[i]) = DRAND();
     }

     /* 
      * fill in other positions of the array, to make sure that
      * fftw doesn't overwrite them 
      */
     for (j = 1; j < istride; ++j)
	  for (i = 0; i < n * howmany; ++i) {
	       c_re(in1[i * istride + j]) = i * istride + j;
	       c_im(in1[i * istride + j]) = i * istride - j;
	  }

     for (j = 1; j < ostride; ++j)
	  for (i = 0; i < n * howmany; ++i) {
	       c_re(out1[i * ostride + j]) = -i * ostride + j;
	       c_im(out1[i * ostride + j]) = -i * ostride - j;
	  }

     plan = fftw_create_plan(n, dir, FFTW_ESTIMATE | wisdom_flags);
     CHECK(plan != NULL, "can't create plan");
     WHEN_VERBOSE(fftw_print_plan(plan));

     /* fft-ize */
     fftw(plan, howmany, in1, istride, n * istride, out1, ostride,
	  n * ostride);

     fftw_destroy_plan(plan);

     /* check for overwriting */
     for (j = 1; j < istride; ++j)
	  for (i = 0; i < n * howmany; ++i)
	       CHECK(c_re(in1[i * istride + j]) == i * istride + j &&
		     c_im(in1[i * istride + j]) == i * istride - j,
		     "input has been overwritten");
     for (j = 1; j < ostride; ++j)
	  for (i = 0; i < n * howmany; ++i)
	       CHECK(c_re(out1[i * ostride + j]) == -i * ostride + j &&
		     c_im(out1[i * ostride + j]) == -i * ostride - j,
		     "input has been overwritten");

     for (i = 0; i < howmany; ++i) {
	  if (dir == FFTW_FORWARD) 
	       fftw_naive(n, in2 + n * i, out2 + n * i);
	  else
	       fftwi_naive(n, in2 + n * i, out2 + n * i);
     }

     /* compute the relative error */
     error = 0.0;
     for (i = 0; i < n * howmany; ++i) {
	  a = sqrt((c_re(out1[i * ostride]) - c_re(out2[i])) *
		   (c_re(out1[i * ostride]) - c_re(out2[i])) +
		   (c_im(out1[i * ostride]) - c_im(out2[i])) *
		   (c_im(out1[i * ostride]) - c_im(out2[i])));
	  a /= sqrt(c_re(out2[i]) * c_re(out2[i]) + c_im(out2[i])
		    * c_im(out2[i]));
	  if (a > error)
	       error = a;
     }
     
     CHECK(error < 1e-8, "wrong answer");
     WHEN_VERBOSE(printf("OK\n"));

     fftw_free(in1);  
     fftw_free(in2);
     fftw_free(out1);
     fftw_free(out2);
     if (!(wisdom_flags & FFTW_USE_WISDOM))
	  fftw_check_memory_leaks();
}

void test_out_of_place_both(int n, int istride, int ostride,
	       int howmany)
{
     printf("TEST CORRECTNESS (out of place, FFTW_FORWARD) n = %d  "
	    "istride = %d  ostride = %d  howmany = %d\n",
	    n, istride, ostride, howmany);
     test_out_of_place(n, istride, ostride, howmany, FFTW_FORWARD);

     printf("TEST CORRECTNESS (out of place, FFTW_BACKWARD) n = %d  "
	    "istride = %d  ostride = %d  howmany = %d\n",
	    n, istride, ostride, howmany);
     test_out_of_place(n, istride, ostride, howmany, FFTW_BACKWARD);
}

void test_in_place(int n, int istride, int howmany, fftw_direction dir)
{
     FFTW_COMPLEX *in1, *in2, *out2;
     fftw_plan plan;
     int i, j;
     double error, a;

     in1 = (FFTW_COMPLEX *)fftw_malloc(istride * n * sizeof(FFTW_COMPLEX) * howmany);
     in2 = (FFTW_COMPLEX *)fftw_malloc(n * sizeof(FFTW_COMPLEX) * howmany);
     out2 = (FFTW_COMPLEX *)fftw_malloc(n * sizeof(FFTW_COMPLEX) * howmany);

     /* generate random inputs */
     for (i = 0; i < n * howmany; ++i) {
	  c_re(in1[i * istride]) = c_re(in2[i]) = DRAND();
	  c_im(in1[i * istride]) = c_im(in2[i]) = DRAND();
     }

     /* 
      * fill in other positions of the array, to make sure that
      * fftw doesn't overwrite them 
      */
     for (j = 1; j < istride; ++j)
	  for (i = 0; i < n * howmany; ++i) {
	       c_re(in1[i * istride + j]) = i * istride + j;
	       c_im(in1[i * istride + j]) = i * istride - j;
	  }

     plan = fftw_create_plan(n, dir, FFTW_IN_PLACE | FFTW_ESTIMATE | 
			     wisdom_flags);
     CHECK(plan != NULL, "can't create plan");
     WHEN_VERBOSE(fftw_print_plan(plan));

     /* fft-ize */
     fftw(plan, howmany, in1, istride, n * istride, (COMPLEX *)NULL, 0,
	  0);

     fftw_destroy_plan(plan);

     /* check for overwriting */
     for (j = 1; j < istride; ++j)
	  for (i = 0; i < n * howmany; ++i)
	       CHECK(c_re(in1[i * istride + j]) == i * istride + j &&
		     c_im(in1[i * istride + j]) == i * istride - j,
		     "input has been overwritten");

     for (i = 0; i < howmany; ++i) {
	  if (dir == FFTW_FORWARD) 
	       fftw_naive(n, in2 + n * i, out2 + n * i);
	  else
	       fftwi_naive(n, in2 + n * i, out2 + n * i);
     }

     /* compute the relative error */
     error = 0.0;
     for (i = 0; i < n * howmany; ++i) {
	  a = sqrt((c_re(in1[i * istride]) - c_re(out2[i])) *
		   (c_re(in1[i * istride]) - c_re(out2[i])) +
		   (c_im(in1[i * istride]) - c_im(out2[i])) *
		   (c_im(in1[i * istride]) - c_im(out2[i])));
	  a /= sqrt(c_re(out2[i]) * c_re(out2[i]) + c_im(out2[i])
		    * c_im(out2[i]));
	  if (a > error)
	       error = a;
     }
     
     CHECK(error < 1e-8, "wrong answer");
     WHEN_VERBOSE(printf("OK\n"));

     fftw_free(in1);  
     fftw_free(in2);
     fftw_free(out2);
     if (!(wisdom_flags & FFTW_USE_WISDOM))
	  fftw_check_memory_leaks();
}

void test_in_place_both(int n, int istride, int howmany)
{
     printf("TEST CORRECTNESS (in place, FFTW_FORWARD) n = %d  "
	    "istride = %d  howmany = %d\n",
	    n, istride, howmany);
     test_in_place(n, istride, howmany, FFTW_FORWARD);

     printf("TEST CORRECTNESS (in place, FFTW_BACKWARD) n = %d  "
	    "istride = %d  howmany = %d\n",
	    n, istride, howmany);
     test_in_place(n, istride, howmany, FFTW_BACKWARD);
}


void test_correctness(int n)
{
     int istride, ostride, howmany;

     for (istride = 1; istride < 4; ++istride) 
	  for (ostride = 1; ostride < 4; ++ostride) 
	       for (howmany = 1; howmany < 5; ++howmany)
		    test_out_of_place_both(n, istride, ostride, howmany);

     for (istride = 1; istride < 4; ++istride) 
	  for (howmany = 1; howmany < 5; ++howmany)
	       test_in_place_both(n, istride, howmany);
}

/* test forever */
void test_all()
{
     int n;

     for (n = 1; ; ++n) {
	  test_correctness(n);
	  if (!(wisdom_flags & FFTW_USE_WISDOM))
	       fftw_check_memory_leaks();
     }
}

/*************************************************
 * planner tests
 *************************************************/     
#define PLANNER_TEST_SIZE 100

void test_planner(void)
{
     /*
      * create and destroy many plans, at random.  Check the
      * garbage-collecting allocator of twiddle factors 
      */
     int i;
     int r, s;
     fftw_plan p[PLANNER_TEST_SIZE];

     for (i = 0; i < PLANNER_TEST_SIZE; ++i) {
	  p[i] = (fftw_plan) 0;
     }

     for (i = 0; i < PLANNER_TEST_SIZE * PLANNER_TEST_SIZE; ++i) {
	  r = rand();
	  if (r < 0)
	       r = -r;
	  r = r % PLANNER_TEST_SIZE;

	  do {
	       s = rand();
	       if (s < 0)
		    s = -s;
	       s = s % 10000;
	  } while (s == 0);

	  if (p[r]) 
	       fftw_destroy_plan(p[r]);

	  p[r] = fftw_create_plan(s, FFTW_FORWARD, FFTW_ESTIMATE | 
				  wisdom_flags);

	  if (i % (PLANNER_TEST_SIZE*PLANNER_TEST_SIZE/20) == 0) {
	       WHEN_VERBOSE(printf("test planner: so far so good\n"));
	       WHEN_VERBOSE(printf("test planner: iteration %d out of %d\n",
				   i, PLANNER_TEST_SIZE * PLANNER_TEST_SIZE));
	  }
     }

     for (i = 0; i < PLANNER_TEST_SIZE; ++i) {
	  if (p[i]) 
	       fftw_destroy_plan(p[i]);
     }
}

static double hack_sum;
static int hack_sum_i;

/*
 * paranoid test to see if time is monotonic.  If not, you are
 * really in trouble
 */
void test_timer_paranoid(void)
{
     fftw_time start_t, end_t;
     double sec;
     int i;

     start_t = fftw_get_time();

     /* waste some time */
     for (i = 0; i < 10000; ++i)
	  hack_sum_i = i;

     end_t = fftw_get_time();
     sec = fftw_time_to_sec(fftw_time_diff(end_t,start_t));
     if (sec < 0.0) {
	  fprintf(stderr,
		  "* PROBLEM: I measured a negative time interval.\n"
		  "* Please make sure you defined the timer correctly\n"
		  "* or contact fftw@theory.lcs.mit.edu for help.\n");
     }
}

void test_timer(void)
{
     double times[32], acc, min_time = 10000.00;
     unsigned long iters, iter;
     int last = 0, i;
     fftw_time last_t;

     test_timer_paranoid();

     last_t = fftw_get_time();

     for (i = 0; i < 32; i++) {
          fftw_time start_t, end_t;
          double sum = 0.0, x = 1.0;
          double sum1 = 0.0, x1 = 1.0;

          iters = 1 << i;

          start_t = fftw_get_time();
          for (iter = 0; iter < iters; ++iter) {
	       /* some random calculations for timing... */
               sum += x; x = .5*x + 0.2*x1; sum1 += x+x1; x1 = .4*x1 + 0.1*x;
               sum += x; x = .5*x + 0.2*x1; sum1 += x+x1; x1 = .4*x1 + 0.1*x;
               sum += x; x = .5*x + 0.2*x1; sum1 += x+x1; x1 = .4*x1 + 0.1*x;
               sum += x; x = .5*x + 0.2*x1; sum1 += x+x1; x1 = .4*x1 + 0.1*x;
          }
          end_t = fftw_get_time();

	  hack_sum = sum;
          times[i] = fftw_time_to_sec(fftw_time_diff(end_t,start_t));

          WHEN_VERBOSE(printf("Number of iterations = 2^%d = %lu, time = %g, "
			      "time/iter = %g\n",
			      i, iters, times[i], 
			      times[i] / iters));

	  /* Paranoia check: make sure time is non-decreasing: */
	  if (fftw_time_to_sec(fftw_time_diff(start_t,last_t)) < 0.0 ||
	      times[i] < 0.0)
	       fprintf(stderr,
		       "* PROBLEM: I measured a negative time interval.\n"
		       "* Please make sure you defined the timer correctly\n"
		       "* or contact fftw@theory.lcs.mit.edu for help.\n");

	  last_t = end_t;

	  last = i;
	  if (times[i] > 10.0) 
	       break;
     }

     /*
      * at this point, `last' is the last valid element in the
      * `times' array.
      */

     for (i = 0; i <= last; ++i)
	  if (times[i] > 0.0 && times[i] < min_time)
	       min_time = times[i];

     printf("\nMinimum resolvable time interval = %g seconds.\n\n",
	    min_time);
     
     for (acc = 0.1; acc > 0.0005; acc *= 0.1) {
          double t_final;
	  t_final = times[last] / (1 << last);
	  
          for (i = last; i >= 0; --i) {
               double t_cur, error;;
               iters = 1 << i;
               t_cur = times[i] / iters;
               error = (t_cur - t_final) / t_final;
               if (error < 0.0)
		    error = -error;
               if (error > acc)
		    break;
	  }

	  ++i;
	  
          printf("Minimum time for %g%% accuracy = %g seconds.\n",
                 acc * 100.0, times[i]);
     }
     printf("\nMinimum time used in FFTW timing (FFTW_TIME_MIN)"
	    " = %g seconds.\n", FFTW_TIME_MIN);
}

/*************************************************
 * help
 *************************************************/     
void usage(void)
{
     printf("Usage:  fftw_test [options]\n");
     printf("  -s <n>    : test speed for size n\n");
     printf("  -c <n>    : test correctness for size n\n");
     printf("  -a        : test correctness for all sizes "
	    "(does not terminate)\n");
     printf("  -p        : test planner\n");
     printf("  -w <file> : use wisdom & read/write it from/to file\n");
     printf("  -t        : test timer resolution\n");
     printf("  -v        : verbose output\n");
     printf("  -h        : this help\n");
#ifndef HAVE_GETOPT
     printf("(When run with no arguments, an interactive mode is used.)\n");
#endif
}

char wfname[128];

void handle_option(char opt, char *optarg)
{
     FILE *wf;
     int n;

     switch (opt) {
	 case 's':
	      n = atoi(optarg);
	      CHECK(n > 0, "-s requires a positive integer argument");
	      test_speed(n);
	      break;
	      
	 case 'c':
	      n = atoi(optarg);
	      CHECK(n > 0, "-c requires a positive integer argument");
	      test_correctness(n);
	      break;
	      
	 case 'p':
	      test_planner();
	      break;
	      
	 case 'a':
	      test_all();
	      break;
	      
	 case 't':
	      test_timer();
	      break;
	      
	 case 'w':
	      wisdom_flags = FFTW_USE_WISDOM;
	      strcpy(wfname,optarg);
	      wf = fopen(wfname,"r");
	      if (wf == 0) {
		   printf("Couldn't open wisdom file \"%s\".\n",wfname);
		   printf("This file will be created upon completion.\n");
	      }
	      else {
		   CHECK(FFTW_SUCCESS == fftw_import_wisdom_from_file(wf),
			 "illegal wisdom file format");
		   fclose(wf);
	      }
	      break;
	      
	 case 'v':
	      verbose = TRUE;
	      break;
	      
	 case 'h':
	      usage();
     }
     
     /* every test must free all the used FFTW memory */
     if (!(wisdom_flags & FFTW_USE_WISDOM))
	  fftw_check_memory_leaks();
}

short askuser(const char *s)
{
     char c;
     char ret;

     printf("%s (y/n) ",s);
     scanf("%c",&c);

     while (c != 'n' && c != 'N' && c != 'y' && c != 'Y')
          scanf("%c",&c);

     /* scan in the return character
        so that it won't screw us up later: */
     scanf("%c",&ret);

     return(c == 'y' || c == 'Y');
}

int zzmain(int argc, char *argv[])
{
     verbose = FALSE;
     wisdom_flags = 0;



     /* To parse the command line, we use getopt, but this
	does not seem to be in the ANSI standard (it is only
	available on UNIX, apparently). */
#ifndef HAVE_GETOPT
     if (argc > 1)
	  printf("Sorry, command-line arguments are not available on\n"
		 "this system.  Run fftw_test with no arguments to\n"
		 "use it in interactive mode.\n");

     if (argc <= 1) {
	  int n = 0;
	  char s[128] = "";

	  usage();

	  printf("\n");

	  if (askuser("Verbose output?"))
	       handle_option('v',"");
	  if (askuser("Use/test wisdom?")) {
	       printf("  Enter wisdom file name to use: ");
	       gets(s);
	       handle_option('w',s);
	  }
	  if (askuser("Test correctness?")) {
	       if (askuser("  -- for all sizes?"))
		    handle_option('a',"");
	       else {
		    printf("  Enter n: ");
		    scanf("%d",&n);
		    sprintf(s,"%d",n);
		    handle_option('c',s);
	       }
	  }
	  if (askuser("Test speed?")) {
	       printf("  Enter n: ");
	       scanf("%d",&n);
	       sprintf(s,"%d",n);
	       handle_option('s',s);
	  }
	  if (askuser("Test planner?"))
	       handle_option('p',"");
	  if (askuser("Test timer?"))
	       handle_option('t',"");
     }

#else /* read command-line args using getopt facility */
     {
	  extern char *optarg;
	  extern int optind;
	  int c;

	  if (argc <= 1) 
	       usage();
	  while ((c = getopt(argc, argv, "s:c:w:patvh")) != -1)
	       handle_option(c,optarg);
	  if (argc != optind)
	       usage();
     }
#endif

     if (wisdom_flags & FFTW_USE_WISDOM) {
	  char *ws;
	  FILE *wf;

	  ws = fftw_export_wisdom_to_string();
	  CHECK(ws != 0,"error exporting wisdom to string");
	  printf("\nAccumulated wisdom:\n     %s\n",ws);
	  fftw_forget_wisdom();
	  CHECK(FFTW_SUCCESS == fftw_import_wisdom_from_string(ws),
		"unexpected error reading in wisdom from string");
	  fftw_free(ws);

	  wf = fopen(wfname,"w");
	  CHECK(wf != 0,"error creating wisdom file");
	  fftw_export_wisdom_to_file(wf);
	  fclose(wf);
     }

     /* make sure to dispose of wisdom before checking for memory leaks */
     fftw_forget_wisdom();

     fftw_check_memory_leaks();

     exit(0);
}
