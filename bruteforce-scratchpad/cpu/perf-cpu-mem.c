#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

// gcc perf-cpu-mem.c -O3 -w -o perf-cpu-mem -lm

typedef uint64_t number_t;



#define INDEX_MAX_NUMBER     10000lu

#define INDEX_MAX_FOURS      INDEX_MAX_NUMBER

#define INDEX_LIST_SIZE      (4*INDEX_MAX_NUMBER)

#define SQRT2                1.41421356237309504880168872

uint32_t count4;
uint32_t index_main[INDEX_MAX_NUMBER+2];
uint32_t index_fours[(INDEX_MAX_FOURS+1)*4];
uint32_t index_linked_lists[INDEX_LIST_SIZE];

uint32_t index_linked_lists_current_top = (2*INDEX_MAX_NUMBER);

int cpu_is_square32(uint32_t sq)
{
	int intsqrt = (uint32_t)roundf(sqrtf((float)sq));
    return intsqrt*intsqrt == sq;
}

void add_number_to_index(uint32_t number, uint32_t four_offset)
{
	uint32_t idx = number*2;
	while (index_linked_lists[idx] !=0 )
		idx = index_linked_lists[idx+1];
	index_linked_lists[idx] = 1+four_offset;                      //actual value +1 to avoid zero
	index_linked_lists[idx+1] = index_linked_lists_current_top;   //pointer to the next free pair
	index_linked_lists_current_top+=2;
}

void add_four_to_index(uint32_t a1,uint32_t a2,uint32_t a3,uint32_t a4)
{
	uint32_t four_offset = count4*4;
	index_fours[four_offset + 0] = a4;
	index_fours[four_offset + 1] = a3;
	index_fours[four_offset + 2] = a2;
	index_fours[four_offset + 3] = a1;
	count4++;

	add_number_to_index(a1, four_offset);
	add_number_to_index(a2, four_offset);
	add_number_to_index(a3, four_offset);
	add_number_to_index(a4, four_offset);



	index_main[a1] = index_main[a1]+1;
	index_main[a2] = index_main[a2]+1;
	index_main[a3] = index_main[a3]+1;
	index_main[a4] = index_main[a4]+1;
}

void cpu_index_add(uint32_t a1)
{
    uint32_t s2_i_start = ceilf(sqrtf((float)a1));                              //cpu_find_index(a1);
    uint32_t s2_i_end = ceilf((float)s2_i_start*SQRT2);  //cpu_find_index(2*a1);
	printf("    a1=%ld\n",a1);
	for (uint32_t s2_i = s2_i_start; s2_i < s2_i_end; s2_i++)
    {
		uint32_t s2 = s2_i*s2_i;//squares[s2_i];
		if (s2 > a1)
		{
			uint32_t a2 = s2 - a1;
			printf("        a2=%ld\n", a2);
			if (a2 < a1)
			{
				uint32_t s3_i_start = ceilf(sqrtf((float)a2));   
				uint32_t s3_i_end = ceilf((float)s3_i_start*SQRT2);
				for (uint32_t s3_i = s3_i_start; s3_i < s3_i_end; s3_i++)
				{
					uint32_t s3 = s3_i*s3_i; //squares[s3_i];
					if (s3 > a2)
					{
						uint32_t a3 = s3 - a2;
						printf("            a3=%ld\n", a3);
						if (a3 < a2 && cpu_is_square32(a1 + a3))
						{
							// found 3
							uint32_t s4_i_start = ceilf(sqrtf((float)a3));
							uint32_t s4_i_end = ceilf((float)s4_i_start*SQRT2);
							for (uint32_t s4_i = s4_i_start; s4_i < s4_i_end; s4_i++)
							{
								uint32_t s4 = s4_i*s4_i; //squares[s4_i];
								if (s4 > a3)
								{
									uint32_t a4 = s4 - a3;
									printf("                a4=%ld\n", a4);
									if (a4 < a3 && cpu_is_square32(a1 + a4) && cpu_is_square32(a2 + a4))
									{
										// found 4
										
										printf("\nfound 4 on cpu: [%lu,%lu,%lu,%lu]\n", a4, a3, a2, a1);
										//add_four_to_index(a1,a2,a3,a4);
										count4++;
										/*
										number_t s5_i_start = floor(sqrt(a4));
										number_t s5_i_end = ceil((double)s5_i_start*SQRT2);
										for (number_t s5_i = s5_i_start; s5_i < s5_i_end; s5_i++)
										{
											number_t s5 = s5_i*s5_i; //squares[s5_i];
											if (s5>a4)
											{
												number_t a5 = s5 - a4;
												if (a5<a4 && cpu_is_square(a1+a5) && cpu_is_square(a2+a5) && cpu_is_square(a3+a5))
												{
													// found 5
													printf("\nfound 5 on cpu: [%lu,%lu,%lu,%lu,%lu]\n", a5, a4, a3, a2, a1);
													number_t s6_i_start = floor(sqrt(a5));
													number_t s6_i_end = ceil((double)s6_i_start*SQRT2);
													for (number_t s6_i = s6_i_start; s6_i < s6_i_end; s6_i++)
													{
														number_t s6 = s6_i*s6_i;//squares[s6_i];
														if (s6>a5)
														{
															number_t a6 = s6 - a5;
															if (a6<a5 && cpu_is_square(a1+a6) && cpu_is_square(a2+a6) && cpu_is_square(a3+a6) && cpu_is_square(a4+a6))
															{
																// found 6 !!
																printf("\n !!!!!!!!!!!!!!!!! found 6 on cpu: [%lu,%lu,%lu,%lu,%lu,%lu]\n", a6, a5, a4, a3, a2, a1);
															}
														}
													}
												}
											}
										}
										*/
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

void cpu_create_index()
{
	count4 = 0;
	for(uint32_t x = 1; x<=INDEX_MAX_NUMBER; x++)
	{
		if (x%10000 == 0)
			printf("%ld:  4s=%ld\n", x, count4);
		cpu_index_add(x);
	}
	

	printf("fours iterated. added %ld fours to index:\n", count4);
	/*
	for(uint32_t f; f<count4; f++)
	{
		//printf("[%ld,%ld,%ld,%ld]\n", index_fours[f*4+0],index_fours[f*4+1],index_fours[f*4+2],index_fours[f*4+3]);
	}
	
	printf("linked list oversize: %ld\n", (index_linked_lists_current_top-2*INDEX_MAX_NUMBER) / 8);
	for(uint32_t x = 1; x<=INDEX_MAX_NUMBER; x++)
	{
		if (index_linked_lists[2*x] != 0)
		{
			printf("list values for %ld:", x);
	        uint32_t idx = 2*x;
	        while (index_linked_lists[idx] !=0 ){
				uint32_t four_offset = index_linked_lists[idx]-1;
				printf("    [%ld] = %ld (%ld,%ld,%ld,%ld) -> %ld\n", idx, index_linked_lists[idx], index_fours[four_offset+0],index_fours[four_offset+1],index_fours[four_offset+2],index_fours[four_offset+3],  index_linked_lists[idx+1]);
		        idx = index_linked_lists[idx+1];
				
			}
		}
	}*/
	
	
	/*
	int max_per = 0;
	uint32_t max_number = 0;
	uint32_t sum = 0;
	for(uint32_t i = 0; i<=INDEX_MAX_NUMBER; i++)
	{
		if (index_main[i] > 0){
			sum += index_main[i];
		    //printf("index_main[%ld]=%ld\n", i, index_main[i]);
		    if (index_main[i] > max_per){
			    max_per = index_main[i];
				max_number = i;
			}
		}
	}
	printf("number with maximum pointers: %ld - %ld pointers. all pointers = %ld\n", max_number, max_per, sum);
	*/
	
}

int main(int argc, char **argv)
{
	printf("start\n");
	number_t start = 1000000000;
	number_t end = 2000000000;
	
	printf("params: %d\n", argc);
	if (argc == 1)
	{
	    printf("Usage: perf-cpu <start> <end>\n");
		printf("or:\n");
		printf("Usage: perf <check-one-number>\n");
		printf("No parameters supplied, using defaults: start=%lu, end=%lu\n", start, end);
	}
	else if (argc == 2)
	{
		printf("Invalid parameters.\n");
		return 1;
	    //cpu_check(atol(argv[1]));
		
	}
	else if (argc == 3)
	{
	    start = atol(argv[1]);
		end = atol(argv[2]);
	}
	else
	{
	    printf("Invalid parameters.\n");
		return 1;
	}
	
	
	printf("creating index up to %ld\n", INDEX_MAX_NUMBER);
	clock_t begin = clock();
	
	cpu_index_add(3362);
    //cpu_create_index(start, end);
	
    clock_t finish = clock();
	double time_spent = (double)(finish - begin) / (double)CLOCKS_PER_SEC;
	printf("index created in: %.10f secs\n", time_spent);
	
    return 0;
}