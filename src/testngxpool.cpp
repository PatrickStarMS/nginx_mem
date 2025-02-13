#include "../include/ngx_mem_pool.h"


typedef struct Data stData;
struct Data
{
    char *ptr;
    FILE *pfile;
};

void func1(char *p1)
{
  char *p = (char *)p1;
  printf("free ptr mem!");
  free(p);
}
void func2(FILE *pf1)
{
  FILE *pf = (FILE *)pf1;
  printf("close file!");
  fclose(pf);
}
int main() { 
    ngx_mem_pool mempool(512);
    
    if(!mempool.ngx_create_pool(512)){
      printf("create pool failed!\n");
      return -1;
    }

    void *p1 =(stData *) mempool.ngx_palloc(512);
    if(nullptr==p1){
      printf("palloc failed!\n");
      return -1;
    }
    stData *p2 = (stData *)mempool.ngx_palloc(sizeof(stData));
    if(nullptr==p2){     
      printf("palloc failed!\n");     
      return -1;
    }
    p2->ptr = (char *)malloc(12);
    strcpy(p2->ptr, "hello world");
    p2->pfile = fopen("data.txt", "w");
    
    ngx_pool_cleanup_s *c1 =mempool.ngx_pool_cleanup_add( sizeof(char*));
    c1->handler = (ngx_pool_cleanup_pt)func1;
    c1->data = p2->ptr;

    ngx_pool_cleanup_s *c2 = mempool.ngx_pool_cleanup_add( sizeof(FILE*));
    c2->handler =(ngx_pool_cleanup_pt) func2;
    c2->data = p2->pfile;

    mempool.ngx_destroy_pool();
    return 0;
}