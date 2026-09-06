#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

// gcc perf-cpu.c -O3 -w -o perf-cpu -lm

typedef uint64_t number_t;

struct batch_params_t {
    number_t start;
    number_t count;
};

typedef struct batch_params_t batch_params_t;

int number_size;
number_t* squares;
number_t squares_count;

number_t* square_roots;
number_t square_roots_count;

// maximal a1 that can be checked
#define MAX_A1 4000000000llu

// number of elements in square lookup table (= ceils(sqrt(2*MAX_A1))
#define SQUARES_COUNT 90000llu

int cpu_is_square(number_t sq)
{
    return squares[(int)round(sqrt(sq))] == sq;
	//number_t intsqrt = (number_t)round(sqrt(sq));
    //return intsqrt*intsqrt == sq;
}

int cpu_is_square_naive(number_t sq)
{
	int intsqrt = (int)round(sqrt(sq));
    return intsqrt*intsqrt == sq;
}

number_t cpu_square_floor(number_t sq)
{
    return (number_t)floor(sqrt(sq));
}

number_t cpu_square_round(number_t sq)
{
    return (number_t)round(sqrt(sq));
}

number_t cpu_find_index(number_t sq)
{
	return (number_t)ceil(sqrt(sq));
}

number_t cpu_binary_search(number_t x)
{
    number_t low = 0;
	number_t high = squares_count-1;
	while (low <= high) {
        int mid = low + (high - low) / 2;

        if (x == squares[mid])
          return mid;

        if (x > squares[mid])
          low = mid + 1;
    else
      high = mid - 1;
  }

  return high;
}

number_t cpu_binary_is_square(number_t x)
{
    number_t index = cpu_binary_search(x);
	return squares[index] == x;
}

number_t cpu_binary_find_index(number_t x)
{
    number_t index = cpu_binary_search(x);
	return (squares[index] >= x) ? index : index+1;
}

void cpu_check(number_t a1)
{
    number_t s2_i_start = cpu_find_index(a1);
    number_t s2_i_end = cpu_find_index(2*a1);
	for (number_t s2_i = s2_i_start; s2_i < s2_i_end; s2_i++)
    {
		number_t s2 = s2_i*s2_i;//squares[s2_i];
		number_t a2 = s2 - a1;
		if (a2 > 0 && a2 < a1)
        {
		    number_t s3_i_start = cpu_find_index(a2);
            number_t s3_i_end = cpu_find_index(2 * a2);
            for (number_t s3_i = s3_i_start; s3_i < s3_i_end; s3_i++)
			{
			    number_t s3 = s3_i*s3_i; //squares[s3_i];
                number_t a3 = s3 - a2;
                if (a3 > 0 && a3 < a2 && cpu_is_square(a1 + a3))
				{
					// found 3
					number_t s4_i_start = cpu_find_index(a3);
					number_t s4_i_end = cpu_find_index(2 * a3);
					for (number_t s4_i = s4_i_start; s4_i < s4_i_end; s4_i++)
					{
						number_t s4 = s4_i*s4_i; //squares[s4_i];
						number_t a4 = s4 - a3;
						if (a4 > 0 && a4 < a3 && cpu_is_square(a1 + a4) && cpu_is_square(a2 + a4))
						{
							// found 4
							number_t s5_i_start = cpu_find_index(a4);
							number_t s5_i_end = cpu_find_index(2 * a4);
							for (number_t s5_i = s5_i_start; s5_i < s5_i_end; s5_i++)
							{
								number_t s5 = s5_i*s5_i; //squares[s5_i];
								number_t a5 = s5 - a4;
								if (a5 > 0 && a5<a4 && cpu_is_square(a1+a5) && cpu_is_square(a2+a5) && cpu_is_square(a3+a5))
								{
								    // found 5
									printf("\nfound 5 on cpu: [%lu,%lu,%lu,%lu,%lu]\n", a5, a4, a3, a2, a1);
									number_t s6_i_start = cpu_find_index(a5);
									number_t s6_i_end = cpu_find_index(2 * a5);
									for (number_t s6_i = s6_i_start; s6_i < s6_i_end; s6_i++)
									{
										number_t s6 = s6_i*s6_i;//squares[s6_i];
										number_t a6 = s6 - a5;
										if (a6 > 0 && a6<a5 && cpu_is_square(a1+a6) && cpu_is_square(a2+a6) && cpu_is_square(a3+a6) && cpu_is_square(a4+a6))
										{
										    // found 6 !!
											printf("\n !!!!!!!!!!!!!!!!! found 6 on cpu: [%lu,%lu,%lu,%lu,%lu,%lu]\n", a6, a5, a4, a3, a2, a1);
										}
									}
								}
							}
						}
					}
				}
			}
		}
    }
}

void *cpu_batch(void* args)
{
    number_t start = ((struct batch_params_t*)args)->start;
	number_t count = ((struct batch_params_t*)args)->count;
    for(number_t a1=start; a1<=start+count; a1++)
	{
	    cpu_check(a1);
    }
	
	//printf("\nthread finished.");
    return 0;
}

void cpu_self_test()
{
	printf("--- testing first 100k on single thread CPU\n");
	time_t start,end;
	time(&start);
	batch_params_t p1;
	p1.start=1;
	p1.count=100000;
    cpu_batch(&p1);
	time(&end);
    double time_spent = difftime(end,start);
	printf("time: %0.0f secs\n\n", time_spent);
	
	int thread_count = 20;
	printf("--- testing first 1mln on %d threads CPU\n", thread_count);
	time(&start);
	pthread_t threads[thread_count];
    number_t checks_per_thread = 1000000 / thread_count;
    for (int i=0; i < thread_count; ++i) 
	{
		struct batch_params_t *p = (struct batch_params_t *)malloc(sizeof(struct batch_params_t));
		p->start = i*checks_per_thread+1;
		p->count = checks_per_thread;
        pthread_create(&threads[i], NULL, cpu_batch, (void*)p);
	}

    for (int i=0; i < thread_count; ++i)
        pthread_join(threads[i], NULL);
		
	time(&end);
    time_spent = difftime(end,start);
	printf("time: %0.0f secs\n\n", time_spent);
}

void cpu_run_batch(number_t start, number_t end, int thread_count)
{
	//printf("batch for %lld .. %lld on %d threads\n", start, end, thread_count);
	batch_params_t p1;
	p1.start=start;
	p1.count=end-start;
	
	pthread_t threads[thread_count];
    number_t checks_per_thread = p1.count / thread_count;
    for (number_t i=0; i < thread_count; ++i) 
	{
		struct batch_params_t *p = (struct batch_params_t *)malloc(sizeof(struct batch_params_t));
		p->start = start + i*checks_per_thread;
		p->count = checks_per_thread;
		if (i == thread_count-1)
			p->count = end - p->start;
		//printf("started thread %d: start=%lld, count=%lld\n", i, p->start, p->count);
        pthread_create(&threads[i], NULL, cpu_batch, (void*)p);
	}

    for (int i=0; i < thread_count; ++i)
        pthread_join(threads[i], NULL);
	
	//printf("\nall threads finished. \n\n");
}

void cpu_run_check(number_t start, number_t end)
{
	printf(" ---- actual start. start=%d, end=%d", start, end);
    number_t batch_size = 5000;
	number_t batch_count = (end-start) / batch_size;
	for(int b = 0; b<batch_count; b++)
	{
		number_t batch_start = start + b*batch_size;
		number_t batch_end = batch_start + batch_size;
		if (b == batch_count-1)
			batch_end = end;
	    printf("."); fflush(stdout);
	    //printf("-------------------- starting batch %lld .. %lld (%d / %d)\n", batch_start, batch_end, b, batch_count);
		cpu_run_batch(batch_start, batch_end, 20);
		//printf("-------------------- batch finished\n");
	}
}

void cpu_test_sqrt_validity(number_t max_test, number_t (*f_checked)(number_t), number_t (*f_expected)(number_t) )
{
	printf("testing validity 0..%lld\n", max_test);
	long errors = 0;
	long max_diff = 0;
	long max_diff_plus = 0;
	long max_diff_minus = 0;
	for(number_t x=0; x<max_test; x++)
	{
		number_t checked = (*f_checked)(x);
		number_t expected = (*f_expected)(x);
		if (checked != expected)
		{
			errors++;
			long diff_abs = abs(expected-checked);
			long diff = expected-checked;

			if (diff_abs > max_diff)
				max_diff = diff_abs;
			if (diff > max_diff_plus)
				max_diff_plus = diff;
			if (diff  < max_diff_minus)
				max_diff_minus = diff;
		}
		
		if (x%10000000 == 0) {
			printf(".");
			fflush(stdout);
		}
	}
	
	printf("\ntesting result: %s, number of errors %d, max_diff = %d [%d,%d] \n", errors==0 ? "OK" : "ERROR", errors, max_diff, max_diff_minus, max_diff_plus);
}

void cpu_test_sqrt_time(number_t max_test, number_t repetitions, number_t (*f_checked)(number_t))
{
	printf("testing time 0..%lld\n", max_test);
	clock_t begin = clock();
	for(number_t x=0; x<max_test; x++)
	{
		for(number_t i=0; i<repetitions; i++){
		    number_t checked = (*f_checked)(x);
            if (checked > 100000000000ull)
		        printf("Something is wrong... %lld,%lld\n", x, checked);
		}

	}
	
    clock_t end = clock();
	double time_spent = (double)(end - begin) / (double)CLOCKS_PER_SEC;
	printf("time: %.10f secs\n", time_spent);
}



number_t cpu_square_naive(number_t x)
{
    return x*x;
}

number_t cpu_square_mem(number_t x)
{
    return squares[x];
}

void cpu_tests()
{
	/*
	printf("\ncpu_square_floor\n");
	cpu_test_sqrt_validity(1000000000,cpu_square_round, cpu_square_round);
    cpu_test_sqrt_time(1000000000,cpu_square_round);
	
	printf("\ncpu_is_square\n");
	cpu_test_sqrt_validity(1000000000,cpu_is_square, cpu_is_square);
    cpu_test_sqrt_time(1000000000,cpu_is_square);
	
	printf("\ncpu_is_square_naive\n");
	cpu_test_sqrt_validity(1000000000,cpu_is_square_naive, cpu_is_square);
    cpu_test_sqrt_time(1000000000,cpu_is_square_naive);
	*/
	printf("\ncpu_square_naive\n");
	cpu_test_sqrt_validity(SQUARES_COUNT, cpu_square_naive, cpu_square_naive);
    cpu_test_sqrt_time(SQUARES_COUNT, 10000000000, cpu_square_naive);
	
	printf("\ncpu_square_mem\n");
	cpu_test_sqrt_validity(SQUARES_COUNT, cpu_square_naive, cpu_square_mem);
    cpu_test_sqrt_time(SQUARES_COUNT, 10000000000, cpu_square_mem);
	
}

void init_square_array()
{
    number_size = sizeof(number_t);
    printf("using %d bytes per number\n", number_size);
	squares_count = (number_t)SQUARES_COUNT;
	printf("max a1=%lu, creating square array size=%lu\n", MAX_A1, squares_count);
	squares = (number_t *)malloc(number_size * squares_count);
	for(number_t i=0; i<squares_count; i++)
	    squares[i] = i*i;
    printf("squares array ready.\n");
	
	/*
	square_roots_count = 1000000000;
    printf("creating sqrt array size=%lu\n", square_roots_count);
	square_roots = (number_t *)malloc(number_size * square_roots_count);
	printf("filling array...");
	for(number_t i=0; i<square_roots_count; i++) {
	    square_roots[i] = floor(sqrt(i));
	}
    printf("squares array ready.\n");
	*/
}


int main(int argc, char **argv)
{
	printf("start\n");
	number_t start = 1;
	number_t end = 1000000000;
	
	printf("params: %d\n", argc);
	if (argc == 1)
	{
	    printf("Usage: perf-cpu <start> <end>\n");
		printf("or:\n");
		printf("Usage: perf <check-one-number>\n");
		printf("No parameters supplied, using defaults: start=%lu, end=%lu\n", start, end);
		init_square_array();
	}
	else if (argc == 2)
	{
	    init_square_array();
	    cpu_check(atol(argv[1]));
		return 0;
	}
	else if (argc == 3)
	{
	    init_square_array();
	    start = atol(argv[1]);
		end = atol(argv[2]);
	}
	else
	{
	    printf("Invalid parameters.\n");
		return 1;
	}
	
	cpu_self_test();
	
	time_t begin,finish;
	time(&begin);
	
    cpu_run_check(start, end);
	
    time(&finish);
	double global_time_spent = difftime(finish,begin);
	printf("time: %.10f secs\n", global_time_spent);
	
    return 0;
}