
#include <stdio.h>

void byteorder()
{
	union {
		char ch;
		int x;
	} test;

	test.x = 1;

	if (test.ch == 1)
		printf("small endian\n");
	else
		printf("big endian \n");

	return;
}

void byteorder2()
{
	union
	{
		short value;   //2bytes, 16bits
		char union_bytes[ sizeof( short ) ];
	} test;

	test.value = 0x0102;  //one byte has 8 bits, 0x0102 = 0000 0001 0000 0010

	if (  ( test.union_bytes[ 0 ] == 1 ) && ( test.union_bytes[ 1 ] == 2 ) )
	{
		printf( "big endian\n" );
	}
	else if ( ( test.union_bytes[ 0 ] == 2 ) && ( test.union_bytes[ 1 ] == 1 ) )
	{
		printf( "little endian\n" );
	}
	else
	{
		printf( "unknown...\n" );
	}
}

int main()
{
	byteorder();
	byteorder2();
	printf("%d\n", sizeof( short ));
	return 0;
}