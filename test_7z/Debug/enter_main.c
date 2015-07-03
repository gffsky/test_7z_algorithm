#include<stdio.h>
#include<malloc.h>
#include"lzmaLib.h"

void Usage()
{
	fputs(
		"Usage:\n"
		"LZMAComp <command> <input> <output>\n"
		"Command:\n"
		"  -c: Compress a single file <input> into <output>.\n"
		"  -d: Decompress a single file <input> into <output>.\n",stderr);
}

int CompressFile(FILE*fpOut,FILE*fpIn,unsigned long InSize);
int DecompressFile(FILE*fpOut,FILE*fpIn);


int main(int argc,char**argv)
{
	if(argc<4)
	{
		Usage();
		return 1;
	}
	if(!stricmp(argv[1],"-c"))//压缩一个文件
	{
		FILE*fp=fopen(argv[2],"rb");
		FILE*fpout=fopen(argv[3],"wb");
		int iRet;
		unsigned long fLen;
		if(!fp)
		{
			fprintf(stderr,"Unable to open %s\n",argv[2]);
			return 2;
		}
		if(!fpout)
		{
			fprintf(stderr,"Unable to write %s\n",argv[3]);
			return 2;
		}
		fseek(fp,0,SEEK_END);
		fLen=ftell(fp);
		fseek(fp,0,SEEK_SET);
		printf("Input file size=%u\n",fLen);
		iRet=CompressFile(fpout,fp,fLen);
		if(iRet)
			fprintf(stderr,"Error:%d\n",iRet);
		fclose(fpout);
		fclose(fp);
		if(iRet)
			unlink(argv[3]);
		return iRet;
	}
	if(!stricmp(argv[1],"-d"))//解压一个文件
	{
		FILE*fp=fopen(argv[2],"rb");
		FILE*fpout=fopen(argv[3],"wb");
		int iRet;
		if(!fp)
		{
			fprintf(stderr,"Unable to open %s\n",argv[2]);
			return 2;
		}
		if(!fpout)
		{
			fprintf(stderr,"Unable to write %s\n",argv[3]);
			return 2;
		}
		iRet=DecompressFile(fpout,fp);
		if(iRet)
			fprintf(stderr,"Error:%d\n",iRet);
		fclose(fpout);
		fclose(fp);
		if(iRet)
			unlink(argv[3]);
		return iRet;
	}
	Usage();
	return 1;
}

int CompressFile(FILE*fpOut,FILE*fpIn,unsigned long InSize)
{
	void*pInBuffer;//输入缓冲区
	void*pOutBuffer;//输出缓冲区
	unsigned long OutSize;//输出缓冲区大小

	unsigned char Props[LZMA_PROPS_SIZE];//属性
	size_t PropsSize=LZMA_PROPS_SIZE;//属性大小

	pInBuffer=malloc(InSize);//缓冲区分配内存
	pOutBuffer=malloc(OutSize=InSize);//输出缓冲区分配和输入缓冲区一样大的内存
	if(!pInBuffer||!pOutBuffer)
	{
		free(pInBuffer);
		free(pOutBuffer);
		return 2;
	}

	fread(pInBuffer,1,InSize,fpIn);//读取文件

	switch(LzmaCompress(//开始压缩
		(unsigned char *)pOutBuffer,(size_t*)&OutSize,//输出缓冲区，大小
		(unsigned char *)pInBuffer,InSize,//输入缓冲区，大小
		(unsigned char*)Props,(size_t*)&PropsSize,//属性，属性大小
		9,0,-1,-1,-1,-1,-1))//压缩比最大。其余全部取默认
	{
	case SZ_OK://成功完成
		fwrite(&InSize,1,sizeof(InSize),fpOut);//写入原数据大小
		fwrite(&OutSize,1,sizeof(OutSize),fpOut);//写入解压后的数据大小
		fwrite(Props,1,PropsSize,fpOut);//写入属性
		fwrite(pOutBuffer,1,OutSize,fpOut);//写入缓冲区
		free(pInBuffer);//释放内存
		free(pOutBuffer);
		return 0;
	case SZ_ERROR_PARAM://参数错误
		free(pInBuffer);
		free(pOutBuffer);
		return 1;
	default:
	case SZ_ERROR_MEM://内存分配错误
	case SZ_ERROR_THREAD://线程错误
		free(pInBuffer);
		free(pOutBuffer);
		return 2;
	case SZ_ERROR_OUTPUT_EOF://缓冲区过小
		free(pInBuffer);
		free(pOutBuffer);
		return 3;
	}
}

int DecompressFile(FILE*fpOut,FILE*fpIn)
{
	void*pSrcBuffer;
	size_t InSize;

	void*pDestBuffer;
	size_t OutSize;

	unsigned char Props[LZMA_PROPS_SIZE];

	fread(&OutSize,1,sizeof(OutSize),fpIn);//读取原数据大小
	fread(&InSize,1,sizeof(InSize),fpIn);//读取压缩后的数据大小

	pDestBuffer=malloc(OutSize);//分配内存
	pSrcBuffer=malloc(InSize);//分配内存
	if(!pSrcBuffer||!pDestBuffer)//内存不足
	{
		free(pSrcBuffer);
		free(pDestBuffer);
		return 2;
	}

	fread(Props,1,sizeof(Props),fpIn);
	fread(pSrcBuffer,1,InSize,fpIn);
	//MY_STDAPI LzmaUncompress(unsigned char *dest, size_t *destLen, const unsigned char *src, SizeT *srcLen,
	//	const unsigned char *props, size_t propsSize);
	switch(LzmaUncompress((unsigned char*)pDestBuffer,&OutSize,(unsigned char*)pSrcBuffer,&InSize,Props,sizeof(Props)))
	{
	case SZ_OK:
		fwrite(pDestBuffer,1,OutSize,fpOut);
		free(pDestBuffer);
		free(pSrcBuffer);
		return 0;
	case SZ_ERROR_DATA:
	case SZ_ERROR_UNSUPPORTED:
	case SZ_ERROR_INPUT_EOF:
		free(pDestBuffer);
		free(pSrcBuffer);
		return 1;
	default:
	case SZ_ERROR_MEM:
		free(pDestBuffer);
		free(pSrcBuffer);
		return 2;
	}
}