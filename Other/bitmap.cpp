/*
如何对某一位置0或者置1？
#define setbit(x,y) x|=(1<<y) //将X的第Y位置1
#define clrbit(x,y) x&=!(1<<y) //将X的第Y位清0
*/

#define BITSPERWORD 32 //数组每个元素32位
#define SHIFT 5
#define MASK 0x1F
#define MAXNUM 10000         //最大整数
const int num = 6000;        //整数个数
int length = 1 + MAXNUM / BITSPERWORD; //最大数值除以32得到数组大小
unsigned int *a = new unsigned int[length];

//i>>SHIFT表示i在数组中的下标 等价于i/32；1<<(i & MASK))等价于i%32，表示第几个bit
//i & MASK == i % 32 == 余数 == i & (32 - 1) == i & MASK 就是对32做MOD求余数 
void set(int i)  {    a[i>>SHIFT] |=  (1<<(i & MASK)); }  //将某bit置1
void clr(int i)  {    a[i>>SHIFT] &= ~(1<<(i & MASK)); }  //将某bit置0

//若第0 bit为1，返回1；若第3 bit为1，则返回8；若第10 bit为1，则返回1024
int  test(int i) { return a[i>>SHIFT] &   (1<<(i & MASK)); }  

int main(void)
{   
    for (int i=0;i!=length;++i)     
        a[i]=0;             //每bit都初始化为0
    srand(time(0));
    int i = 0;
    while(i != num)        //生成num个大小不超过MAXNUM的整数
    {
        int p = rand() % MAXNUM;
        if (!test(p))
        {
            set(p);
            ++i;
        }
    }

    for (i=0; i<MAXNUM; ++i)       //按升序打印出这些整数
    {
        if(test(i))
            cout<<i<<endl;
    }

    return 0;
}
