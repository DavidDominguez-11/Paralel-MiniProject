#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef enum { EMPTY, PLANT, HERBIVORE, CARNIVORE } Species;
typedef struct { Species species; int energy, age, hunger; } Cell;
typedef enum { ACT_STAY, ACT_MOVE, ACT_EAT, ACT_BREED, ACT_DIE } ActionKind;
typedef struct { ActionKind kind; int target; int breed_target; } Action;
typedef struct { int rows, cols, ticks, threads, plants, herbivores, carnivores; uint64_t seed; } Config;
static const int DR[4] = {-1, 0, 1, 0}, DC[4] = {0, 1, 0, -1};

static uint64_t mix64(uint64_t x) {
    x += UINT64_C(0x9e3779b97f4a7c15);
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    return x ^ (x >> 31);
}
static uint64_t rv(uint64_t seed, int tick, int index, int salt) {
    return mix64(seed ^ (uint64_t)(tick + 1) * UINT64_C(0x632be59bd9b4e019) ^
                 (uint64_t)(index + 1) * UINT64_C(0x8cb92baa3f3a5b1d) ^ (uint64_t)(salt + 1));
}
static int chance(uint64_t seed, int tick, int index, int salt, int percent) { return rv(seed,tick,index,salt)%100 < (uint64_t)percent; }
static int neigh(const Config *c, int index, int d) {
    int r=index/c->cols, col=index%c->cols, nr=r+DR[d], nc=col+DC[d];
    return nr<0 || nr>=c->rows || nc<0 || nc>=c->cols ? -1 : nr*c->cols+nc;
}
static int choose(const Config *c, const Cell *g, int index, Species wanted, uint64_t seed, int tick, int salt) {
    int a[4], n=0;
    for (int d=0; d<4; ++d) { int j=neigh(c,index,d); if (j>=0 && (wanted==EMPTY ? g[j].species==EMPTY : g[j].species==wanted)) a[n++]=j; }
    return n ? a[rv(seed,tick,index,salt)%n] : -1;
}
static void decide(const Config *c, const Cell *g, Action *a, int i, int tick, uint64_t seed) {
    Cell x=g[i]; a->kind=ACT_DIE; a->target=-1; a->breed_target=-1; if (x.species==EMPTY) return;
    int age=x.age+1, hunger=x.hunger+1, energy=x.energy-1;
    int maxage=x.species==PLANT ? 1000000 : x.species==HERBIVORE ? 50 : 60;
    int maxhunger=x.species==PLANT ? 1000000 : x.species==HERBIVORE ? 3 : 4;
    if (age>=maxage || hunger>=maxhunger || (x.species!=PLANT && energy<=0)) return;
    a->kind=ACT_STAY; a->target=i;
    if (x.species==PLANT) { int j=choose(c,g,i,EMPTY,seed,tick,10); if (j>=0 && chance(seed,tick,i,11,30)) { a->kind=ACT_BREED; a->breed_target=j; } return; }
    Species food=x.species==HERBIVORE ? PLANT : HERBIVORE; int j=choose(c,g,i,food,seed,tick,20);
    if (j>=0) { a->kind=ACT_EAT; a->target=j; return; }
    if (x.species==HERBIVORE && choose(c,g,i,CARNIVORE,seed,tick,21)>=0) { j=choose(c,g,i,EMPTY,seed,tick,22); if (j>=0) { a->kind=ACT_MOVE; a->target=j; } return; }
    j=choose(c,g,i,EMPTY,seed,tick,23); if (j>=0) { a->kind=ACT_MOVE; a->target=j; }
    int threshold=x.species==HERBIVORE ? 8 : 12, probability=x.species==HERBIVORE ? 20 : 15;
    if (energy>=threshold && x.hunger==0 && chance(seed,tick,i,24,probability)) { j=choose(c,g,i,EMPTY,seed,tick,25); if (j>=0) { a->kind=ACT_BREED; a->target=i; a->breed_target=j; } }
}
static int priority(const Cell *g, const Action *a, int target) { return a->kind==ACT_EAT && g[target].species!=EMPTY ? 0 : (a->kind==ACT_MOVE || a->kind==ACT_EAT || a->kind==ACT_BREED ? 1 : 2); }
static void apply_actions(const Config *c, const Cell *g, Cell *next, const Action *actions) {
    int total=c->rows*c->cols, *winner=malloc((size_t)total*sizeof(*winner));
    for (int i=0;i<total;++i) { next[i]=(Cell){EMPTY,0,0,0}; winner[i]=-1; }
    for (int i=0;i<total;++i) for (int k=0;k<2;++k) { int t=k ? actions[i].breed_target : actions[i].target; if (t<0) continue; int p=priority(g,&actions[i],t); if (winner[t]<0 || p<priority(g,&actions[winner[t]],t) || (p==priority(g,&actions[winner[t]],t) && i<winner[t])) winner[t]=i; }
    for (int i=0;i<total;++i) { const Cell *s=&g[i]; const Action *a=&actions[i]; if (s->species==EMPTY || a->kind==ACT_DIE || winner[a->target]!=i) continue; Cell r=*s; r.age++; if (s->species != PLANT) {
      r.hunger++;
      if (a->kind != ACT_EAT) r.energy--;
  } if (a->kind==ACT_EAT) { r.energy+=s->species==HERBIVORE?1:2; r.hunger=0; } next[a->target]=r; if (a->kind==ACT_BREED && a->breed_target>=0 && winner[a->breed_target]==i) next[a->breed_target]=(Cell){s->species,s->species==PLANT?1:3,0,0}; }
    free(winner);
}
static void print_state(const Config *c, const Cell *g, int tick, FILE *out) {
    int p=0,h=0,ca=0,total=c->rows*c->cols; for (int i=0;i<total;++i) { p+=g[i].species==PLANT; h+=g[i].species==HERBIVORE; ca+=g[i].species==CARNIVORE; }
    fprintf(out,"Tick %d\nPlantas: %d\nHerbívoros: %d\nCarnívoros: %d\nDistribución:\n",tick,p,h,ca);
    for (int r=0;r<c->rows;++r) { for (int col=0;col<c->cols;++col) { Species s=g[r*c->cols+col].species; fputc(s==PLANT?'P':s==HERBIVORE?'H':s==CARNIVORE?'C':'.',out); if(col+1<c->cols) fputc(' ',out); } fputc('\n',out); } fputc('\n',out);
}
static void usage(const char *n) { fprintf(stderr,"Uso: %s [--rows N] [--cols N] [--ticks N] [--plants N] [--herbivores N] [--carnivores N] [--threads N] [--seed N] [--output ARCHIVO]\n",n); }
int main(int argc,char **argv) {
    Config c={20,40,20,1,150,40,15,UINT64_C(20240820)}; const char *output=NULL;
    for(int i=1;i<argc;i+=2) { if(i+1>=argc){usage(argv[0]);return 2;} long v=strtol(argv[i+1],NULL,10); if(!strcmp(argv[i],"--rows"))c.rows=v;else if(!strcmp(argv[i],"--cols"))c.cols=v;else if(!strcmp(argv[i],"--ticks"))c.ticks=v;else if(!strcmp(argv[i],"--plants"))c.plants=v;else if(!strcmp(argv[i],"--herbivores"))c.herbivores=v;else if(!strcmp(argv[i],"--carnivores"))c.carnivores=v;else if(!strcmp(argv[i],"--threads"))c.threads=v;else if(!strcmp(argv[i],"--seed"))c.seed=(uint64_t)v;else if(!strcmp(argv[i],"--output"))output=argv[i+1];else{usage(argv[0]);return 2;} }
    int total=c.rows*c.cols; if(c.rows<=0||c.cols<=0||c.ticks<0||c.threads<=0||c.plants<0||c.herbivores<0||c.carnivores<0||c.plants+c.herbivores+c.carnivores>total){fprintf(stderr,"Configuración inválida.\n");return 2;}
    Cell *g=calloc((size_t)total,sizeof(*g)),*next=calloc((size_t)total,sizeof(*next)); Action *actions=calloc((size_t)total,sizeof(*actions)); if(!g||!next||!actions)return 1;
    int counts[3]={c.plants,c.herbivores,c.carnivores},placed=0; for(int s=0;s<3;++s)for(int n=0;n<counts[s];++n){int pos=rv(c.seed,-1,placed+n,s)%total;while(g[pos].species!=EMPTY)pos=(pos+1)%total;Species sp=s+1;g[pos]=(Cell){sp,sp==PLANT?1:sp==HERBIVORE?5:7,0,0};placed++;}
    omp_set_num_threads(c.threads); FILE *out=output?fopen(output,"w"):stdout; if(!out){perror(output);return 1;}
    fprintf(out,"Configuración: %dx%d, ticks=%d, threads=%d, seed=%llu\n\n",c.rows,c.cols,c.ticks,c.threads,(unsigned long long)c.seed); print_state(&c,g,0,out);
    for(int tick=1;tick<=c.ticks;++tick){
        #pragma omp parallel for schedule(static)
        for(int i=0;i<total;++i)decide(&c,g,&actions[i],i,tick,c.seed);
        apply_actions(&c,g,next,actions); Cell *tmp=g;g=next;next=tmp; print_state(&c,g,tick,out);
    }
    if (output) fclose(out);
    free(g);
    free(next);
    free(actions);
    return 0;
}
