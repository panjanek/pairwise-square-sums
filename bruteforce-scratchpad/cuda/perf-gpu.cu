#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>

typedef uint64_t number_t;

struct batch_params {
    number_t start;
    number_t count;
};

int number_size;

static __device__ __forceinline__ unsigned long long int umul_wide (unsigned int a, unsigned int b)
{
    unsigned long long int r;
    asm ("mul.wide.u32 %0,%1,%2;\n\t" : "=l"(r) : "r"(a), "r"(b));
    return r;
}

static __device__ __forceinline__ uint32_t isqrtll (uint64_t a)   //exact floor up to 10mld
{
    uint64_t rem, arg;
    uint32_t b, r, s, scal;

    arg = a;
    /* Normalize argument */
    scal = __clzll (a) & ~1;
    a = a << scal;
    b = a >> 32;
    /* Approximate rsqrt accurately. Make sure it's an underestimate! */
    float fb, fr;
    fb = (float)b;
    asm ("rsqrt.approx.ftz.f32 %0,%1; \n\t" : "=f"(fr) : "f"(fb));
    r = (uint32_t) fmaf (1.407374884e14f, fr, -438.0f);
    /* Compute sqrt(a) as a * rsqrt(a) */
    s = __umulhi (r, b);
    /* NR iteration combined with back multiply */
    s = s * 2;
    rem = a - umul_wide (s, s);
    r = __umulhi ((uint32_t)(rem >> 32) + 1, r);
    s = s + r;
    /* Denormalize result */
    s = s >> (scal >> 1);
    /* Make sure we get the floor correct; can be off by one to either side */
    rem = arg - umul_wide (s, s);
    if ((int64_t)rem < 0) s--;
    else if (rem >= ((uint64_t)s * 2 + 1)) s++;
    return (arg == 0) ? 0 : s;
}

static __device__ __forceinline__ int gpu_is_square(number_t x)
{
	number_t floor_sqrt = isqrtll(x);
	return floor_sqrt * floor_sqrt == x;
}

static __device__ __forceinline__ number_t gpu_mulsq2_double(number_t x)
{
    return ceil((double)x*1.41421356237309504880168872);
}

__device__ __forceinline__ number_t gpu_check(number_t a1)
{
    number_t result = 1;
    number_t s2_i_start = isqrtll(a1);
    number_t s2_i_end = gpu_mulsq2_double(s2_i_start); //gpu_find_index_high(2*a1);
	for (number_t s2_i = s2_i_start; s2_i < s2_i_end; s2_i++)
    {
		number_t s2 = s2_i*s2_i;
		number_t a2 = s2 - a1;
		if (a2 > 0 && a2 < a1)
        {
		    number_t s3_i_start = isqrtll(a2);
            number_t s3_i_end = gpu_mulsq2_double(s3_i_start);  //gpu_find_index_high(2 * a2);
            for (number_t s3_i = s3_i_start; s3_i < s3_i_end; s3_i++)
			{
			    number_t s3 = s3_i * s3_i; 
                number_t a3 = s3 - a2;
                if (a3 > 0 && a3 < a2 && gpu_is_square(a1 + a3))
				{
					// found 3
					number_t s4_i_start = isqrtll(a3);
					number_t s4_i_end = gpu_mulsq2_double(s4_i_start); //gpu_find_index_high(2 * a3);
					for (number_t s4_i = s4_i_start; s4_i < s4_i_end; s4_i++)
					{
						number_t s4 = s4_i * s4_i; 
						number_t a4 = s4 - a3;
						if (a4 > 0 && a4 < a3 && gpu_is_square(a1 + a4) && gpu_is_square(a2 + a4))
						{
							// found 4
							number_t s5_i_start = isqrtll(a4);
							number_t s5_i_end = gpu_mulsq2_double(s5_i_start); //gpu_find_index_high(2 * a4);
							for (number_t s5_i = s5_i_start; s5_i < s5_i_end; s5_i++)
							{
								number_t s5 = s5_i * s5_i;
								number_t a5 = s5 - a4;
								if (a5 > 0 && a5<a4 && gpu_is_square(a1+a5) && gpu_is_square(a2+a5) && gpu_is_square(a3+a5))
								{
								    // found 5
									printf("\nfound 5 on GPU: [%lu,%lu,%lu,%lu,%lu]\n", a5, a4, a3, a2, a1);
									result = 5;
									number_t s6_i_start = isqrtll(a5);
									number_t s6_i_end = gpu_mulsq2_double(s6_i_start); //gpu_find_index_high(2 * a5);
									for (number_t s6_i = s6_i_start; s6_i < s6_i_end; s6_i++)
									{
										number_t s6 = s6_i*s6_i; //gpu_squares[s6_i];
										number_t a6 = s6 - a5;
										if (a6 > 0 && a6<a5 && gpu_is_square(a1+a6) && gpu_is_square(a2+a6) && gpu_is_square(a3+a6) && gpu_is_square(a4+a6))
										{
										    // found 6 !!
											printf("\n !!!!!!!!!!!!!!!!! found 6 on GPU: [%lu,%lu,%lu,%lu,%lu,%lu]\n", a6, a5, a4, a3, a2, a1);
											return 6;
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
	
	return result;
}

__global__ void kernel(number_t* results, batch_params* p)
{
    number_t i = (number_t)(threadIdx.x + blockIdx.x * blockDim.x);
	if (i < p->count)
	{
	    number_t a1 = i + p->start;
	    number_t r = gpu_check(a1);
	    results[i] = r;
    }
}

void gpu_start(number_t start, number_t end)
{
    // 8192,128 : 7.9s
    int nblocks = 4096;
    int blocksize = 256;
	number_t count = nblocks * blocksize;

	printf("GPU start. From %lu to %lu in %lu batches\n", start, end, count);
    printf("preparing data for GPU\n");
		
	//results array
	cudaError_t error;
    number_t* cpu_results = (number_t *)malloc(number_size * count);
    number_t* gpu_results;
	error = cudaMalloc((void**)&gpu_results,count * number_size);
	if (error != cudaSuccess) 
	    printf("cudaMalloc: %d - %s\n\n", error, cudaGetErrorName(error));
	
	//params
	struct batch_params *cpu_params = (struct batch_params *)malloc(sizeof(struct batch_params));
	batch_params* gpu_params;
    error = cudaMalloc((void**)&gpu_params,sizeof(struct batch_params));
	if (error != cudaSuccess) 
	    printf("cudaMalloc: %d - %s\n\n", error, cudaGetErrorName(error));
	
    clock_t global_start,global_end, batch_start, batch_end;
	global_start = clock();
	cpu_params->start = start;
	cpu_params->count = count;
	int found5 = 0;
	int found6 = 0;
	printf("entering main loop\n");
	fflush(stdout);
    while(cpu_params->start < end)
	{
	    batch_start = clock();
	    //copy results --> device
	    memset(cpu_results, 0, count * number_size);
	    error = cudaMemcpy(gpu_results, cpu_results, count * number_size, cudaMemcpyHostToDevice);
		if (error != cudaSuccess) 
	        printf("cudaMemcpy: %d - %s\n\n", error, cudaGetErrorName(error));
		
		//copy parameters --> device
        error = cudaMemcpy(gpu_params, cpu_params, sizeof(struct batch_params), cudaMemcpyHostToDevice);
		if (error != cudaSuccess)
	        printf("cudaMemcpy: %d - %s\n\n", error, cudaGetErrorName(error));
		
		printf("starting kernels <%d/%d> start=%lu, count=%lu...", nblocks, blocksize, cpu_params->start, cpu_params->count);
		fflush(stdout);
		kernel<<<nblocks, blocksize>>>(gpu_results, gpu_params);
		error = cudaPeekAtLastError();
		if (error != cudaSuccess)
		    printf("cudaPeekAtLastError: %d - %s\n\n", error, cudaGetErrorName(error));
		error = cudaDeviceSynchronize();
		if (error != cudaSuccess)
		    printf("cudaDeviceSynchronize: %d - %s\n\n", error, cudaGetErrorName(error));
		
		batch_end = clock();
		double batch_time_spent = (double)(batch_end - batch_start) / (double)CLOCKS_PER_SEC;
	    printf("done in %.10f secs (total: %.10f s) \n", batch_time_spent, (double)(batch_end-global_start) / (double)CLOCKS_PER_SEC);
		fflush(stdout);
		
		error = cudaMemcpy(cpu_results, gpu_results, count * number_size, cudaMemcpyDeviceToHost);
		if (error != cudaSuccess)
		    printf("cudaMemcpy: %d - %s\n\n", error, cudaGetErrorName(error));
		
		for(number_t k=0; k<count; k++)
		{
		    if (cpu_results[k] == 0)
			    printf("ERROR - results[%lu] skipped!\n", k);
				
			if (cpu_results[k]>=5)
			{
			    number_t a1 = cpu_params->start + k;
				if (cpu_results[k] == 5)
				    found5++;
			    if (cpu_results[k] == 6)
				    found6++;
					
			    printf("found new %lu : a1=%lu (total:%d,%d)\n", cpu_results[k], a1, found5, found6);
				fflush(stdout);
			}
		}
	
		cpu_params->start = cpu_params->start + count;
	}
	
	global_end = clock();
	double global_time_spent = (double)(global_end - global_start) / (double)CLOCKS_PER_SEC;
	printf("Done. fives=%d, sixes=%d, total time: %.10f secs\n\n", found5, found6, global_time_spent);
}

void gpu_tune()
{
    int N = 1 << 20; // example size (1M elements)

    int minGridSize = 0;   // suggested minimum grid size
    int blockSize = 0;     // threads per block

    // Query optimal block size
    cudaOccupancyMaxPotentialBlockSize(
        &minGridSize,       // out: minimum grid size needed
        &blockSize,         // out: block size (threads per block)
        kernel,             // kernel function
        0,                  // dynamic shared memory per block (bytes)
        0                   // block size limit (0 = no limit)
    );

    // Compute grid size to cover all elements
    int gridSize = (N + blockSize - 1) / blockSize;
	
	printf("CUDA tuning: blockSize=%d, minGridSize=%d, gridSize=%d\n", blockSize, minGridSize, gridSize);
}

int main(int argc, char **argv)
{
    // p2.xlarge
	// https://forums.developer.nvidia.com/t/integer-square-root/198642
	// nvcc -Xptxas -O3 perf-gpu.cu -o perf-gpu
	// nvcc -Xptxas -O3 --use_fast_math perf.cu -o perf
	// watch -n0.1 nvidia-smi
	// 1mld .. 1017170432 - tested
	
	// nvcc -arch=sm_89 -O3 -Xptxas -O3,-v,-dlcm=ca perf-gpu.cu -o perf-gpu
	
	number_size = sizeof(number_t);
	gpu_tune();
	
	number_t start = 1000000000;
	number_t end = 2000000000;
	
	printf("params: %d\n", argc);
	if (argc == 1)
	{
	    printf("Usage: perf <start> <end>\n");
		printf("or:\n");
		printf("Usage: perf <check-one-number>\n");
		printf("No parameters supplied, using defaults: start=%lu, end=%lu\n", start, end);
	}
	else if (argc == 2)
	{
	    printf("Invalid parameters.\n");
		return 1;
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
	
	gpu_start(start, end);
	
    return 0;
}