#include <stdio.h>
#include <ctype.h>
 
#define N 256
#define M 10 //暂未使用
 
/* 成绩 */
int score[N];

int main()
{
    int n,m;
 
	/* 输入正整数N和M，分别代表学生的数目和操作的数目 */
    printf("input student number and operation number:");
	scanf("%d %d", &n, &m);
    printf("student number: %d,  operation number: %d\n", n, m);
    
	/* 输入学生成绩 */
	int i = 0; 
	while(i < n)
    {
        printf("input student %d score.\n", i);
        
        int temp;
        scanf("%d", &temp);
        printf("score: %d\n", temp);
		score[i++] = temp;
    }
   
    int j=0;
    for (j=0; j<n; ++j)
        printf("student %d , score: %d \n", j + 1, score[j]);
        
    return 0;
}
