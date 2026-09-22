/*
 * s4.c -- search for sets of distinct positive integers whose pairwise sums are all squares,
 *         organised around 4-sets.
 *
 * Idea: a 4-set {a,b,c,d} has S = a+b+c+d and S = (a+b)+(c+d) = (a+c)+(b+d) = (a+d)+(b+c),
 * i.e. S has three representations as a sum of two squares.  Conversely, ANY three distinct
 * representations S = x1+(S-x1) = x2+(S-x2) = x3+(S-x3) (x_i squares) give the rational 4-set
 *     a = (x1+x2+x3-S)/2, b = (x1-x2-x3+S)/2, c = (-x1+x2-x3+S)/2, d = (-x1-x2+x3+S)/2
 * (two essentially different 4-sets per triple of representations).  So enumerating S together with
 * its representations gives EVERY 4-set with a+b+c+d < S_hi in ~linear time.
 * Each 4-set is then extended by one element e (e+a, e+b, e+c, e+d all squares) using
 *     e+a = u^2, e+b = w^2  =>  (u-w)(u+w) = a-b,
 * enumerating divisor pairs of a-b.  a-b = r1^2 - r2^2 for two known roots, so factoring is trivial.
 * Two extensions e,f with e+f square => 6-set.
 *
 * Coverage: every 5-set whose four smallest elements sum to < S_hi, every 6-set whose four smallest
 * elements sum to < S_hi (so in particular every 6-set with a3 < S_hi/4, and every one with a1 < S_hi/4).
 *
 * build: gcc -O3 -march=native -o s4 s4.c -lm -lpthread
 * usage: ./s4 S_lo S_hi [threads=auto] [log2 chunk=20] [--out FILE] [--ckpt FILE] [--resume]
 *                                                     [--noext] [--verify] [--dump4 N] [--test-sixset]
 *        threads: omitted or 0 = number of online CPUs; when resuming, the checkpoint's thread count is used.
 *
 * Output lines (stdout, or --out FILE in append mode):
 *   5: [a1,a2,a3,a4,a5] S=...        a 5-set (largest first); each 5-set is reported exactly once
 *   M: [4-set] ext=[e,f,...] S=...   a 4-set contained in >= 2 different 5-sets (pseudo-solution)
 *   !!!!!!!! FOUND 6-SET: [...]      also written immediately + fsync to SIXSET_FOUND.txt and to stderr
 * Persistence: --ckpt FILE writes (atomically, every 5 s) the next chunk start of every thread plus the
 * cumulative counters; results already flushed to --out are fsynced before each checkpoint.  Restart
 * the identical command with --resume to continue; a chunk interrupted mid-way is redone (so a few
 * duplicate result lines are possible right after a crash; dedupe with sort -u).  A finished range is
 * marked DONE in the checkpoint and a --resume rerun exits at once.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>

/* Override only in the capacity-failure tests. Never truncate a census. */
#ifndef S4_MAX_REPS
#define S4_MAX_REPS 512
#endif
#ifndef S4_MAX_EXT
#define S4_MAX_EXT 64
#endif
#ifndef S4_MAX_DIVS
#define S4_MAX_DIVS 16384
#endif

static void fatal(const char *message){
    fprintf(stderr,"FATAL: %s; this interval is NOT certified complete\n",message);
    fflush(stderr); exit(2);
}

typedef unsigned __int128 u128;
typedef uint64_t u64;
typedef int64_t  i64;
typedef uint32_t u32;

static u64 S_lo = 1, S_hi = 10000000ULL;
static int nthreads = 0;               /* 0 = auto: number of online CPUs (sysconf); a positive 3rd argument overrides */
static u64 W = 1ull << 20;
static int opt_noext = 0, opt_verify = 0, opt_resume = 0, opt_test6 = 0;
static long opt_dump4 = 0;
static const char *opt_out = NULL, *opt_ckpt = NULL;
static FILE *out_fp = NULL;
static volatile int test6_fired = 0;

static u32 *spf = NULL, *quot = NULL; static u64 spf_n = 0;   /* spf[m] = smallest prime factor, quot[m] = m/spf[m] */
static uint16_t *ndivtab = NULL;                                /* ndivtab[m] = number of divisors of m */
static uint8_t sq64[64], sq63[63], sq65[65];
static pthread_mutex_t out_mtx = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    u64 S_done, pairs, S_ge3, triples, sets4, sets5, sets6, ext_cand, ext_sqrt, multi;
    u64 hist5[100];
    u64 region_lo, region_hi;
    double secs;
    volatile u64 chunks_done;
    volatile u64 next_S0;          /* start of the next chunk to process (checkpoint position) */
    char *buf; size_t blen, bcap;  /* per-chunk output buffer */
    long dumped;
    int id;
    u64 max_reps, max_divisors, max_extensions;
} stats_t;

static double now_sec(void){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts); return ts.tv_sec + ts.tv_nsec*1e-9; }

static inline u64 isqrt64(u64 n){
    u64 r=(u64)sqrt((double)n); if(r>0xFFFFFFFFull) r=0xFFFFFFFFull;   /* keep r*r and (r+1)^2 free of overflow */
    while(r*r>n) r--;
    while(r<0xFFFFFFFFull && (r+1)*(r+1)<=n) r++;
    return r;
}
static inline u64 isqrt128(u128 n){
    if(n < ((u128)1<<64)) return isqrt64((u64)n);
    u64 r=(u64)sqrtl((long double)n);
    while((u128)r*r>n) r--;
    while((u128)(r+1)*(r+1)<=n) r++;
    return r;
}
static inline int is_sq128(u128 n){ u64 r=isqrt128(n); return (u128)r*r==n; }

static void u128_str(char *buf, u128 v){
    char tmp[48]; int i=0; if(v==0){ strcpy(buf,"0"); return; }
    while(v){ tmp[i++]=(char)('0'+(int)(v%10)); v/=10; }
    int j=0; while(i) buf[j++]=tmp[--i]; buf[j]=0;
}

/* ---------- output buffering / durable alerts ---------- */
static void buf_append(stats_t *st, const char *s){
    size_t n=strlen(s);
    if(st->blen+n+1 > st->bcap){ st->bcap=(st->blen+n+1)*2+65536; st->buf=realloc(st->buf,st->bcap); if(!st->buf){ perror("realloc buf"); exit(1); } }
    memcpy(st->buf+st->blen,s,n); st->blen+=n; st->buf[st->blen]=0;
}
static void flush_buf(stats_t *st){
    if(st->blen==0) return;
    pthread_mutex_lock(&out_mtx);
    if(fwrite(st->buf,1,st->blen,out_fp)!=st->blen || fflush(out_fp)!=0) fatal("result output failed");
    pthread_mutex_unlock(&out_mtx);
    st->blen=0;
}
static void durable_alert(const char *line){
    /* a 6-set (or the --test-sixset drill): write at once, fsync, to results file + SIXSET_FOUND.txt + stderr */
    pthread_mutex_lock(&out_mtx);
    FILE *f=fopen("SIXSET_FOUND.txt","a");
    if(f){ fputs(line,f); fflush(f); fsync(fileno(f)); fclose(f); }
    fputs(line,out_fp); fflush(out_fp); fsync(fileno(out_fp));
    fputs(line,stderr); fflush(stderr);
    pthread_mutex_unlock(&out_mtx);
}

/* ---------- factorisation / divisors ---------- */
typedef struct { u64 p[24]; int e[24]; int n; } fact_t;
static void fact_add(fact_t *f, u64 m){
    while(m>1){
        u64 pr = spf[m]; int e=0;
        do { m = quot[m]; e++; } while(m>1 && spf[m]==pr);      /* division-free walk */
        int i; for(i=0;i<f->n;i++) if(f->p[i]==pr){ f->e[i]+=e; break; }
        if(i==f->n){ f->p[f->n]=pr; f->e[f->n]=e; f->n++; }
    }
}
static int gen_divisors(const fact_t *f, u64 *out, int max){
    int n=1; out[0]=1;
    for(int i=0;i<f->n;i++){
        int cur=n; u64 pk=1;
        for(int e=1;e<=f->e[i];e++){ pk*=f->p[i]; for(int j=0;j<cur;j++){ if(n>=max) return -1; out[n++]=out[j]*pk; } }
    }
    return n;
}

/* ---------- extension of a 4-set ---------- */
/* v[0..3] distinct positive elements, rt[i][j] = sqrt(v[i]+v[j]).  Writes extensions e into out. */
static int extend4(const u64 *v, const u32 rt[4][4], u128 *out, int maxout, stats_t *st){
    /* pick the pair (i,j) whose difference D = v[i]-v[j] = rt[i][k]^2 - rt[j][k]^2 = (ra-rb)(ra+rb) has the
       fewest divisors, estimated cheaply as d(ra-rb)*d(ra+rb) from a precomputed table; factor only that one */
    static const int pr[6][3] = {{0,1,2},{2,3,0},{0,2,1},{1,3,0},{0,3,1},{1,2,0}};
    u64 best=~0ull; int bi=0,bj=1,bk=2;
    for(int t=0;t<6;t++){
        int i=pr[t][0], j=pr[t][1], k=pr[t][2];
        if(v[i]<v[j]){ int s=i; i=j; j=s; }
        u64 ra=rt[i][k], rb=rt[j][k];
        u64 est=(u64)ndivtab[ra-rb]*ndivtab[ra+rb];
        if(est<best){ best=est; bi=i; bj=j; bk=k; }
    }
    int i=bi, j=bj;
    u64 A=v[i];                          /* A > B,  D = A-B */
    fact_t fbest; fbest.n=0; fact_add(&fbest, (u64)rt[i][bk]-rt[j][bk]); fact_add(&fbest, (u64)rt[i][bk]+rt[j][bk]);
    u64 divs[S4_MAX_DIVS]; int nd = gen_divisors(&fbest, divs, S4_MAX_DIVS);
    if(nd<0) fatal("divisor capacity exceeded");
    if((u64)nd>st->max_divisors) st->max_divisors=(u64)nd;
    int o1=-1,o2=-1; for(int t=0;t<4;t++) if(t!=i && t!=j){ if(o1<0) o1=t; else o2=t; }
    u64 C1=v[o1], C2=v[o2];
    i64 d1 = (i64)C1 - (i64)A, d2 = (i64)C2 - (i64)A;      /* e + C = u^2 + d */
    u64 a1_64=(u64)((d1%64+64)%64), a2_64=(u64)((d2%64+64)%64);
    u64 a1_63=(u64)((d1%63+63)%63), a2_63=(u64)((d2%63+63)%63);
    u64 a1_65=(u64)((d1%65+65)%65), a2_65=(u64)((d2%65+65)%65);
    int cnt=0;
    for(int t=0;t<nd;t++){
        /* divisors are generated in mixed-radix order, so the complementary divisor of divs[t] is divs[nd-1-t] */
        u64 fd = divs[t], g = divs[nd-1-t];
        if(fd >= g) continue;
        if((fd ^ g) & 1) continue;
        u64 u = (fd+g)>>1;
        st->ext_cand++;
        u64 um = u & 63; if(!sq64[(um*um + a1_64) & 63] || !sq64[(um*um + a2_64) & 63]) continue;
        um = u % 63;     if(!sq63[(um*um + a1_63) % 63] || !sq63[(um*um + a2_63) % 63]) continue;
        um = u % 65;     if(!sq65[(um*um + a1_65) % 65] || !sq65[(um*um + a2_65) % 65]) continue;
        u128 usq = (u128)u*u; if(usq <= A) continue;
        u128 e = usq - A;
        if(e==v[0]||e==v[1]||e==v[2]||e==v[3]) continue;
        st->ext_sqrt++;
        if(!is_sq128(e + C1)) continue;
        if(!is_sq128(e + C2)) continue;
        if(cnt>=maxout) fatal("extension capacity exceeded");
        out[cnt]=e;
        cnt++;
    }
    if((u64)cnt>st->max_extensions) st->max_extensions=(u64)cnt;
    return cnt;
}

static int verify_set(const u128 *x, int n){
    for(int i=0;i<n;i++) for(int j=i+1;j<n;j++){ if(x[i]==x[j]) return 0; if(!is_sq128(x[i]+x[j])) return 0; }
    return 1;
}
/* sort descending and format "tag [a,b,...] S=... [verify note]\n" */
static void format_set(char *dst, size_t cap, const char *tag, u128 *x, int n, u64 S){
    for(int i=0;i<n;i++) for(int j=i+1;j<n;j++) if(x[j]>x[i]){ u128 t=x[i]; x[i]=x[j]; x[j]=t; }
    char num[48]; size_t len=0;
    len+=(size_t)snprintf(dst+len,cap-len,"%s [",tag);
    for(int i=0;i<n;i++){ u128_str(num,x[i]); len+=(size_t)snprintf(dst+len,cap-len,"%s%s",num,i<n-1?",":""); }
    snprintf(dst+len,cap-len,"] S=%llu%s\n",(unsigned long long)S, verify_set(x,n)?"":"  *** VERIFY FAILED ***");
}

/* ---------- worker ---------- */
typedef struct { u32 *qnext, *qsave; uint8_t *inited; uint16_t *cnt; u32 *off; u32 *pv; u64 pvcap; } work_t;

static void handle_4set(u64 S, i64 a, i64 b, i64 c, i64 d, u32 r_ab, u32 r_ac, u32 r_ad, u32 r_bc, u32 r_bd, u32 r_cd, stats_t *st){
    u64 v[4] = {(u64)a,(u64)b,(u64)c,(u64)d};
    u32 rt[4][4] = {{0,r_ab,r_ac,r_ad},{r_ab,0,r_bc,r_bd},{r_ac,r_bc,0,r_cd},{r_ad,r_bd,r_cd,0}};
    char line[4096];
    if(opt_verify){
        for(int i=0;i<4;i++) for(int j=i+1;j<4;j++) if((u64)rt[i][j]*rt[i][j] != v[i]+v[j] || v[i]==v[j]){
            snprintf(line,sizeof line,"BAD 4-set S=%llu [%lld,%lld,%lld,%lld]\n",(unsigned long long)S,(long long)a,(long long)b,(long long)c,(long long)d);
            buf_append(st,line); flush_buf(st); fatal("quadruple verification failed"); }
    }
    st->sets4++;
    /* NOTE: a 4-set with all elements divisible by k^2 may still extend to a PRIMITIVE 5-set
       (the odd-one-out families, e.g. residues (0,0,0,0,1) mod 4), so non-primitive 4-sets cannot be skipped. */
    if(opt_dump4 && st->dumped < opt_dump4){
        st->dumped++;
        snprintf(line,sizeof line,"4: [%lld,%lld,%lld,%lld] S=%llu\n",(long long)a,(long long)b,(long long)c,(long long)d,(unsigned long long)S);
        buf_append(st,line);
    }
    if(opt_noext) return;
    u128 ext[S4_MAX_EXT];
    int ne = extend4(v, rt, ext, S4_MAX_EXT, st);
    if(ne==0) return;
    u64 vmax = v[0]; for(int i=1;i<4;i++) if(v[i]>vmax) vmax=v[i];
    if(ne>=2){
        /* a 4-set lying in >=2 different 5-sets: the raw material for a 6-set */
        st->multi++;
        char num[48]; size_t len=0;
        len+=(size_t)snprintf(line+len,sizeof line-len,"M: [%llu,%llu,%llu,%llu] ext=[",(unsigned long long)v[0],(unsigned long long)v[1],(unsigned long long)v[2],(unsigned long long)v[3]);
        for(int t=0;t<ne && len<sizeof line-64;t++){ u128_str(num,ext[t]); len+=(size_t)snprintf(line+len,sizeof line-len,"%s%s",num,t<ne-1?",":""); }
        snprintf(line+len,sizeof line-len,"] S=%llu\n",(unsigned long long)S);
        buf_append(st,line);
        if(opt_test6 && !test6_fired){          /* drill: exercise the 6-set alert path with a NON-solution */
            test6_fired=1;
            u128 x[6]={v[0],v[1],v[2],v[3],ext[0],ext[1]};
            format_set(line,sizeof line,"TEST-6SET (NOT A SOLUTION, --test-sixset drill):",x,6,S);
            durable_alert(line);
        }
    }
    for(int t=0;t<ne;t++){
        if(ext[t] > vmax){
            st->sets5++;
            int lg=0; u128 e=ext[t]; while(e>1){ e>>=1; lg++; } if(lg>99) lg=99; st->hist5[lg]++;
            u128 x[5]={v[0],v[1],v[2],v[3],ext[t]};
            format_set(line,sizeof line,"5:",x,5,S); buf_append(st,line);
        }
    }
    for(int t=0;t<ne;t++) for(int s=t+1;s<ne;s++){
        if(is_sq128(ext[t]+ext[s])){
            st->sets6++;
            u128 x[6]={v[0],v[1],v[2],v[3],ext[t],ext[s]};
            format_set(line,sizeof line,"!!!!!!!! FOUND 6-SET:",x,6,S);
            durable_alert(line);
        }
    }
}

static void process_chunk(u64 S0, u64 S1, work_t *w, stats_t *st){
    u64 n = S1 - S0;
    u64 pmax = isqrt64((S1-1)/2);
    memset(w->cnt, 0, n*sizeof(uint16_t));
    /* pass 1: count */
    for(u64 p=1;p<=pmax;p++){
        if(!w->inited[p]){
            u64 q=p;
            if(S0 > 2*p*p){ q = isqrt64(S0 - p*p); if(q*q < S0 - p*p) q++; }
            w->qnext[p]=(u32)q; w->inited[p]=1;
        }
        u64 q=w->qnext[p]; w->qsave[p]=(u32)q;
        u64 S=p*p+q*q;
        while(S<S1){
            if(w->cnt[S-S0]==UINT16_MAX) fatal("representation counter overflow");
            w->cnt[S-S0]++; S+=2*q+1; q++;
        }
        w->qnext[p]=(u32)q;
    }
    /* pass 2: offsets */
    u64 T=0; for(u64 i=0;i<n;i++){
        w->off[i]=(u32)T; T+=w->cnt[i];
        if(T>UINT32_MAX) fatal("representation offset overflow");
    }
    st->pairs += T;
    if(T > w->pvcap){ w->pvcap = T + T/4 + 1024; w->pv = realloc(w->pv, w->pvcap*sizeof(u32)); if(!w->pv){ perror("realloc"); exit(1);} }
    /* pass 3: fill */
    for(u64 p=1;p<=pmax;p++){
        u64 q=w->qsave[p]; u64 S=p*p+q*q;
        while(S<S1){ w->pv[w->off[S-S0]++]=(u32)p; S+=2*q+1; q++; }
    }
    /* pass 4: process S with >=3 representations */
    u64 pl[S4_MAX_REPS], ql[S4_MAX_REPS], P[S4_MAX_REPS], Q[S4_MAX_REPS];
    for(u64 i=0;i<n;i++){
        unsigned k=w->cnt[i]; st->S_done++;
        if(k>st->max_reps) st->max_reps=k;
        if(k<3) continue;
        u64 S=S0+i;
        if((S&3)==2) continue;                 /* 4-subsets of 5-/6-sets never have S = 2 mod 4 */
        st->S_ge3++;
        u64 start = i? w->off[i-1] : 0;
        if(k>S4_MAX_REPS) fatal("representation capacity exceeded");
        for(unsigned t=0;t<k;t++){ u64 p=w->pv[start+t]; pl[t]=p; P[t]=p*p; Q[t]=S-P[t]; ql[t]=isqrt64(Q[t]); }
        int Sodd = (int)(S&1);
        for(unsigned x=0;x<k;x++) for(unsigned y=x+1;y<k;y++) for(unsigned z=y+1;z<k;z++){
            st->triples++;
            int par = (int)((pl[x]+pl[y]+pl[z])&1);
            /* class A: (Q_x,Q_y,Q_z) needs par==0 ; class B: (Q_x,Q_y,P_z) needs par==Sodd */
            for(int cls=0;cls<2;cls++){
                if(cls==0 && par!=0) continue;
                if(cls==1 && (par!=Sodd || Q[z]==P[z])) continue;   /* S=2p^2: the two classes coincide */
                i64 x1=(i64)Q[x], x2=(i64)Q[y], x3 = cls==0 ? (i64)Q[z] : (i64)P[z];
                i64 SS=(i64)S;
                i64 a=(x1+x2+x3-SS)/2, b=(x1-x2-x3+SS)/2, c=(-x1+x2-x3+SS)/2, d=(-x1-x2+x3+SS)/2;
                if(a<=0||b<=0||c<=0||d<=0) continue;
                u32 r_ab=(u32)ql[x], r_ac=(u32)ql[y], r_cd=(u32)pl[x], r_bd=(u32)pl[y];
                u32 r_ad = cls==0 ? (u32)ql[z] : (u32)pl[z];
                u32 r_bc = cls==0 ? (u32)pl[z] : (u32)ql[z];
                handle_4set(S,a,b,c,d,r_ab,r_ac,r_ad,r_bc,r_bd,r_cd,st);
            }
        }
    }
}

static void *worker(void *arg){
    stats_t *st=(stats_t*)arg;
    double t0=now_sec();
    u64 pmax_all = isqrt64((S_hi-1)/2)+2;
    work_t w; memset(&w,0,sizeof w);
    w.qnext=calloc(pmax_all+2,sizeof(u32)); w.qsave=calloc(pmax_all+2,sizeof(u32)); w.inited=calloc(pmax_all+2,1);
    w.cnt=malloc(W*sizeof(uint16_t)); w.off=malloc((W+1)*sizeof(u32));
    w.pvcap = W/2+4096; w.pv=malloc(w.pvcap*sizeof(u32));
    if(!w.qnext||!w.qsave||!w.inited||!w.cnt||!w.off||!w.pv){ perror("malloc"); exit(1); }
    st->next_S0 = st->region_lo;
    for(u64 S0=st->region_lo; S0<st->region_hi; S0+=W){
        u64 S1 = S0+W; if(S1>st->region_hi) S1=st->region_hi;
        process_chunk(S0,S1,&w,st);
        flush_buf(st);              /* results of this chunk reach the kernel before the chunk counts as done */
        st->next_S0 = S1;
        st->chunks_done++;
    }
    st->secs=now_sec()-t0;
    free(w.qnext); free(w.qsave); free(w.inited); free(w.cnt); free(w.off); free(w.pv);
    return NULL;
}

/* ---------- checkpointing ---------- */
static void write_ckpt(stats_t *st, int done_all){
    if(!opt_ckpt) return;
    u64 *pos=malloc(nthreads*sizeof(u64));
    for(int t=0;t<nthreads;t++) pos[t]=st[t].next_S0;          /* capture positions FIRST ... */
    pthread_mutex_lock(&out_mtx); fflush(out_fp); fsync(fileno(out_fp)); pthread_mutex_unlock(&out_mtx);  /* ... then make results durable */
    char tmp[4200]; snprintf(tmp,sizeof tmp,"%s.tmp",opt_ckpt);
    FILE *f=fopen(tmp,"w"); if(!f){ perror("checkpoint open"); free(pos); return; }
    fprintf(f,"s4ckpt v1\n%llu %llu %d %llu\n",(unsigned long long)S_lo,(unsigned long long)S_hi,nthreads,(unsigned long long)W);
    for(int t=0;t<nthreads;t++)
        fprintf(f,"%d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n", t,
            (unsigned long long)pos[t],(unsigned long long)st[t].region_hi,
            (unsigned long long)st[t].S_done,(unsigned long long)st[t].pairs,(unsigned long long)st[t].S_ge3,(unsigned long long)st[t].triples,
            (unsigned long long)st[t].sets4,(unsigned long long)st[t].sets5,(unsigned long long)st[t].sets6,(unsigned long long)st[t].multi,
            (unsigned long long)st[t].ext_cand,(unsigned long long)st[t].ext_sqrt);
    if(done_all) fprintf(f,"DONE\n");
    fflush(f); fsync(fileno(f)); fclose(f);
    if(rename(tmp,opt_ckpt)!=0) perror("checkpoint rename");
    free(pos);
}
/* thread count recorded in an existing checkpoint (regions depend on it), or -1 */
static int ckpt_thread_count(void){
    FILE *f=fopen(opt_ckpt,"r"); if(!f) return -1;
    char hdr[64]; unsigned long long a,b,d; int c=-1;
    if(!fgets(hdr,sizeof hdr,f) || fscanf(f,"%llu %llu %d %llu",&a,&b,&c,&d)!=4) c=-1;
    fclose(f); return c;
}
/* returns 0 = no checkpoint, 1 = positions restored, 2 = range already complete */
static int read_ckpt(stats_t *st){
    FILE *f=fopen(opt_ckpt,"r"); if(!f) return 0;
    char hdr[64];
    if(!fgets(hdr,sizeof hdr,f) || strncmp(hdr,"s4ckpt v1",9)){ fprintf(stderr,"checkpoint %s: bad header\n",opt_ckpt); exit(1); }
    unsigned long long a,b,d; int c;
    if(fscanf(f,"%llu %llu %d %llu\n",&a,&b,&c,&d)!=4 || a!=S_lo || b!=S_hi || c!=nthreads || d!=W){
        fprintf(stderr,"checkpoint %s was written for S_lo=%llu S_hi=%llu threads=%d chunk=%llu; current parameters differ\n",opt_ckpt,a,b,c,d); exit(1); }
    for(int t=0;t<nthreads;t++){
        int id; unsigned long long ns,rh,v[10];
        if(fscanf(f,"%d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",&id,&ns,&rh,&v[0],&v[1],&v[2],&v[3],&v[4],&v[5],&v[6],&v[7],&v[8],&v[9])!=13
           || id!=t || rh!=st[t].region_hi){ fprintf(stderr,"checkpoint %s: bad line for thread %d\n",opt_ckpt,t); exit(1); }
        st[t].region_lo = ns > st[t].region_hi ? st[t].region_hi : ns;
        st[t].S_done=v[0]; st[t].pairs=v[1]; st[t].S_ge3=v[2]; st[t].triples=v[3]; st[t].sets4=v[4]; st[t].sets5=v[5];
        st[t].sets6=v[6]; st[t].multi=v[7]; st[t].ext_cand=v[8]; st[t].ext_sqrt=v[9];
    }
    char tail[16]; int done = (fgets(tail,sizeof tail,f)!=NULL && !strncmp(tail,"DONE",4));
    fclose(f);
    return done?2:1;
}

int main(int argc, char **argv){
    int pos=0;
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"--noext")) opt_noext=1;
        else if(!strcmp(argv[i],"--verify")) opt_verify=1;
        else if(!strcmp(argv[i],"--resume")) opt_resume=1;
        else if(!strcmp(argv[i],"--test-sixset")) opt_test6=1;
        else if(!strcmp(argv[i],"--dump4") && i+1<argc) opt_dump4=atol(argv[++i]);
        else if(!strcmp(argv[i],"--out") && i+1<argc) opt_out=argv[++i];
        else if(!strcmp(argv[i],"--ckpt") && i+1<argc) opt_ckpt=argv[++i];
        else { pos++; if(pos==1) S_lo=strtoull(argv[i],0,10); else if(pos==2) S_hi=strtoull(argv[i],0,10);
               else if(pos==3) nthreads=atoi(argv[i]); else if(pos==4) W=1ull<<atoi(argv[i]); }
    }
    if(S_lo<1) S_lo=1;
    if(nthreads<=0){                                   /* auto-detect; a resumed checkpoint dictates its own count */
        long n=sysconf(_SC_NPROCESSORS_ONLN); nthreads = n>0 ? (int)n : 1;
        if(opt_ckpt && opt_resume){ int c=ckpt_thread_count(); if(c>0) nthreads=c; }
    }
    if(opt_ckpt && !opt_resume && access(opt_ckpt,F_OK)==0){
        fprintf(stderr,"checkpoint %s exists: add --resume to continue it, or delete it to start over\n",opt_ckpt); return 1; }
    if(opt_out){ out_fp=fopen(opt_out,"a"); if(!out_fp){ perror(opt_out); return 1; } } else out_fp=stdout;

    for(int i=0;i<64;i++) sq64[(i*i)&63]=1;
    for(int i=0;i<63;i++) sq63[(i*i)%63]=1;
    for(int i=0;i<65;i++) sq65[(i*i)%65]=1;
    /* smallest-prime-factor table up to 2*sqrt(S_hi)+64 (root sums) */
    spf_n = 2*isqrt64(S_hi)+64;
    spf = malloc(spf_n*sizeof(u32)); if(!spf){ perror("malloc spf"); return 1; }
    for(u64 i=0;i<spf_n;i++) spf[i]=(u32)i;
    for(u64 i=2;i*i<spf_n;i++) if(spf[i]==i) for(u64 j=i*i;j<spf_n;j+=i) if(spf[j]==j) spf[j]=(u32)i;
    quot = malloc(spf_n*sizeof(u32)); if(!quot){ perror("malloc quot"); return 1; }
    quot[0]=0; quot[1]=1; for(u64 i=2;i<spf_n;i++) quot[i]=(u32)(i/spf[i]);
    ndivtab = calloc(spf_n,sizeof(uint16_t)); if(!ndivtab){ perror("calloc ndiv"); return 1; }
    for(u64 d=1;d<spf_n;d++) for(u64 m=d;m<spf_n;m+=d) ndivtab[m]++;

    stats_t *st=calloc(nthreads,sizeof(stats_t));
    pthread_t *th=malloc(nthreads*sizeof(pthread_t));
    u64 total=S_hi-S_lo;
    for(int t=0;t<nthreads;t++){
        st[t].id=t;
        st[t].region_lo = S_lo + (u64)((u128)total*t/nthreads);
        st[t].region_hi = S_lo + (u64)((u128)total*(t+1)/nthreads);
    }
    int rs = (opt_ckpt && opt_resume) ? read_ckpt(st) : 0;
    if(rs==2){ fprintf(stderr,"checkpoint %s marks S in [%llu, %llu) as complete; nothing to do\n",opt_ckpt,(unsigned long long)S_lo,(unsigned long long)S_hi); return 0; }
    fprintf(stderr,"s4: S in [%llu, %llu), %d threads, chunk %llu, ext=%s%s%s\n",(unsigned long long)S_lo,(unsigned long long)S_hi,nthreads,(unsigned long long)W,opt_noext?"off":"on",
        opt_ckpt?", checkpointing":"", rs==1?", RESUMED from checkpoint":"");
    u64 total_chunks=0;
    for(int t=0;t<nthreads;t++){
        total_chunks += (st[t].region_hi-st[t].region_lo + W-1)/W;
        if(rs==1) fprintf(stderr,"  thread %2d resumes at S=%llu (region end %llu)\n",t,(unsigned long long)st[t].region_lo,(unsigned long long)st[t].region_hi);
        st[t].next_S0 = st[t].region_lo;
        if(pthread_create(&th[t],NULL,worker,&st[t])!=0) fatal("pthread_create failed");
    }
    double t0=now_sec();
    int last_report=-1, last_ckpt=-1;
    for(;;){
        u64 done=0; for(int t=0;t<nthreads;t++) done+=st[t].chunks_done;
        if(done>=total_chunks) break;
        sleep(1);
        double el=now_sec()-t0;
        int cslot=(int)el/5;
        if(cslot!=last_ckpt){ last_ckpt=cslot; write_ckpt(st,0); }
        int slot=(int)el/30;
        if(slot!=last_report && slot>0){
            last_report=slot;
            u64 s4=0,s5=0,s6=0; for(int t=0;t<nthreads;t++){ s4+=st[t].sets4; s5+=st[t].sets5; s6+=st[t].sets6; }
            fprintf(stderr,"[%.0fs] chunks %llu/%llu  4-sets %llu  5-sets %llu  6-sets %llu\n",el,(unsigned long long)done,(unsigned long long)total_chunks,(unsigned long long)s4,(unsigned long long)s5,(unsigned long long)s6);
            fflush(stderr);
        }
    }
    for(int t=0;t<nthreads;t++) pthread_join(th[t],NULL);
    double el=now_sec()-t0;
    write_ckpt(st,1);
    stats_t tot; memset(&tot,0,sizeof tot);
    for(int t=0;t<nthreads;t++){
        tot.S_done+=st[t].S_done; tot.pairs+=st[t].pairs; tot.S_ge3+=st[t].S_ge3; tot.triples+=st[t].triples;
        tot.sets4+=st[t].sets4; tot.sets5+=st[t].sets5; tot.sets6+=st[t].sets6; tot.ext_cand+=st[t].ext_cand; tot.ext_sqrt+=st[t].ext_sqrt; tot.multi+=st[t].multi;
        if(st[t].max_reps>tot.max_reps) tot.max_reps=st[t].max_reps;
        if(st[t].max_divisors>tot.max_divisors) tot.max_divisors=st[t].max_divisors;
        if(st[t].max_extensions>tot.max_extensions) tot.max_extensions=st[t].max_extensions;
        for(int i=0;i<100;i++) tot.hist5[i]+=st[t].hist5[i];
        fprintf(stderr,"  thread %2d: S [%llu,%llu) 4-sets %llu 5-sets %llu  %.1fs\n",t,(unsigned long long)st[t].region_lo,(unsigned long long)st[t].region_hi,(unsigned long long)st[t].sets4,(unsigned long long)st[t].sets5,st[t].secs);
    }
    fprintf(stderr,"TOTAL: S_done=%llu pairs=%llu S_ge3=%llu triples=%llu 4-sets=%llu 5-sets=%llu 6-sets=%llu multi-ext-4sets=%llu ext_cand=%llu ext_sqrt=%llu  wall=%.1fs%s\n",
        (unsigned long long)tot.S_done,(unsigned long long)tot.pairs,(unsigned long long)tot.S_ge3,(unsigned long long)tot.triples,(unsigned long long)tot.sets4,(unsigned long long)tot.sets5,(unsigned long long)tot.sets6,(unsigned long long)tot.multi,(unsigned long long)tot.ext_cand,(unsigned long long)tot.ext_sqrt,el,
        rs==1?" (counters cumulative over resumed runs; wall time of this run only)":"");
    fprintf(stderr,"5-sets by log2(largest element)%s:", rs==1?" (this run only)":"");
    for(int i=0;i<100;i++) if(tot.hist5[i]) fprintf(stderr," %d:%llu",i,(unsigned long long)tot.hist5[i]);
    fprintf(stderr,"\n");
    fprintf(stderr,"CAPACITY: reps=%llu/%d divisors=%llu/%d extensions=%llu/%d%s\n",
        (unsigned long long)tot.max_reps,S4_MAX_REPS,(unsigned long long)tot.max_divisors,S4_MAX_DIVS,
        (unsigned long long)tot.max_extensions,S4_MAX_EXT,rs==1?" (maxima this resumed invocation only)":"");
    if(out_fp!=stdout) fclose(out_fp);
    return 0;
}
