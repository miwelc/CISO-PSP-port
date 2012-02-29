/**************************************************************************
Compressed ISO9660 conveter by BOOSTER
Ported to the PSP and modded for improve the performance on it by BLACKSITH
**************************************************************************/

#include "conf.h"

PSP_MODULE_INFO("Ciso", 0x1000, 1, 0);

PSP_MAIN_THREAD_ATTR(0);

//#define DEBUG_FILENAME

char *fname_in=NULL, fname_out[256];
SceUID fin, fout;
z_stream z;

unsigned int *index_buf = NULL;
unsigned int *crc_buf = NULL;
unsigned char *block_buf1 = NULL;
unsigned char *block_buf2 = NULL;
unsigned char *prebufwr = NULL, *prebufre = NULL;

int level=1;
int numblocks=5000;

/****************************************************************************
	sceIo version of ftell
****************************************************************************/
long sceIoFTell(SceUID *stream) {
  long n, ret;

  ret = (((n = sceIoLseek(*stream, 0, SEEK_CUR)) >= 0) ? (long)n : -1L);

  return (ret);
}

/****************************************************************************
	Get file name
****************************************************************************/
void getfilename(char *dest, char *ori) {
	int i, j=0;
	
	for(i=0; ori[strlen(ori)-1-i] != '/'; i++) ;
	i--;
	dest[i+1] = '\0';
	while(i>=0) {
		dest[j] = ori[(strlen(ori)-1-i)];
		i--;
		j++;
	}
}

/****************************************************************************
	Free memory
****************************************************************************/
void freemem(void) {
	if(index_buf) {
		free(index_buf);
		index_buf = NULL;
	}
	if(crc_buf) {
		free(crc_buf);
		crc_buf = NULL;
	}
	if(block_buf1) {
		free(block_buf1);
		block_buf1 = NULL;
	}
	if(block_buf2) {
		free(block_buf2);
		block_buf2 = NULL;
	}
	if(prebufwr) {
		free(prebufwr);
		prebufwr = NULL;
	}
	if(prebufre) {
		free(prebufre);
		prebufre = NULL;
	}
	if(fname_out) {
		fname_out[0] = '\0';
	}
	if(fname_in) {
		fname_in[0] = '\0';
	}
}


/****************************************************************************
	compress ISO to CSO
****************************************************************************/

CISO_H ciso;
int ciso_total_block;

unsigned long long check_file_size(SceUID *fp)
{
	unsigned long long pos;

	if( sceIoLseek(*fp, 0, SEEK_END) < 0)
		return -1;
	pos = sceIoFTell(fp);
	if(pos==-1) return pos;

	// init ciso header
	memset(&ciso,0,sizeof(ciso));

	ciso.magic[0] = 'C';
	ciso.magic[1] = 'I';
	ciso.magic[2] = 'S';
	ciso.magic[3] = 'O';
	ciso.ver      = 0x01;

	ciso.block_size  = 0x800; // ISO9660 one of sector
	ciso.total_bytes = pos;
#if 0
	// align >0 has bug 
	for(ciso.align = 0 ; (ciso.total_bytes >> ciso.align) >0x80000000LL ; ciso.align++);
#endif

	ciso_total_block = pos / ciso.block_size ;

	sceIoLseek(*fp, 0, SEEK_SET);

	return pos;
}

/****************************************************************************
	decompress CSO to ISO
****************************************************************************/
int decomp_ciso(SceUID fin, SceUID fout)
{
	unsigned int index , index2;
	unsigned long long read_size;
	int dir1, dir2;
	int index_size;
	int block;
	int cmp_size;
	int status;
	int num=0;
	int plain;
	int j=0, temp=0;
	int total_size_offs_wr=0, total_size_offs_re=0;
	int numblocks_temp=0;
	char inf_name_in[MAX_CHAR_FILENAME_INF], inf_name_out[MAX_CHAR_FILENAME_INF];

	// read header
	if(sceIoRead(fin, &ciso, sizeof(ciso)) != sizeof(ciso))
	{
		printf(file_read_err);
		return 1;
	}

	// check header
	if(
		ciso.magic[0] != 'C' ||
		ciso.magic[1] != 'I' ||
		ciso.magic[2] != 'S' ||
		ciso.magic[3] != 'O' ||
		ciso.block_size ==0  ||
		ciso.total_bytes == 0
	)
	{
		printf(file_form_err);
		return 1;
	}
	ciso_total_block = ciso.total_bytes / ciso.block_size;
	if(ciso_total_block<numblocks) numblocks_temp=ciso_total_block;
		else numblocks_temp=numblocks;

	// allocate index block
	index_size = (ciso_total_block + 1 ) * sizeof(unsigned long);
	index_buf  = malloc(index_size);
	block_buf1 = malloc(ciso.block_size);
	block_buf2 = malloc(ciso.block_size*2);
	prebufwr = malloc(ciso.block_size*(numblocks_temp+1));
	prebufre = malloc(ciso.block_size*(numblocks_temp+1));
	dir1 = (int)&prebufwr[0];
	dir2 = (int)&prebufre[0];

	if( !index_buf || !block_buf1 || !block_buf2 || !prebufwr || !prebufre)
	{
		printf(mem_alloc_err);
		return 1;
	}
	memset(index_buf,0,index_size);

	// read index block
	if( sceIoRead(fin, index_buf, index_size) != index_size )
	{
		printf(file_read_err);
		return 1;
	}

	// show info
	getfilename(inf_name_in, fname_in);
	getfilename(inf_name_out, fname_out);
	printf(dec_file_file,inf_name_in,inf_name_out);
	printf(inf_file_size,ciso.total_bytes);
	printf(inf_blck_size,ciso.block_size);
	printf(inf_blck_tota,ciso_total_block);
	printf(inf_indx_algn,1<<ciso.align);

	// init zlib
	z.zalloc = Z_NULL;
	z.zfree  = Z_NULL;
	z.opaque = Z_NULL;

	for(block = 0;block < ciso_total_block ; block++)
	{
		pspDebugScreenSetXY(0, 9);
		if(num++==15) {
			printf(dec_blck_blck, block, ciso_total_block);
			num=0;
		}

		if(num>40) num=0;
		
		if (inflateInit2(&z,-15) != Z_OK)
		{
			printf(infl_init_err, (z.msg) ? z.msg : "???");
			return 1;
		}
		
		if(j==0) {
			total_size_offs_re=0;
			if(numblocks_temp>(ciso_total_block-block)) numblocks_temp = (ciso_total_block-block);
			for(temp=block; temp<=block+numblocks_temp; temp++) {
				// check index
				index  = index_buf[temp];
				plain  = index & 0x80000000;
				index  &= 0x7fffffff;
				if(plain)
					read_size = ciso.block_size;
				else
				{
					index2 = index_buf[temp+1] & 0x7fffffff;
					read_size = (index2-index) << (ciso.align);
				}
				total_size_offs_re += read_size;
			}
			sceIoLseek(fin, ((index_buf[block]&0x7fffffff)<<(ciso.align)), SEEK_SET);// (..., offset, ...)
			pspDebugScreenSetXY(0, 11);
			printf(inf_stat_cach);
			if(sceIoRead(fin, prebufre, total_size_offs_re) != total_size_offs_re && (ciso_total_block-block)>numblocks_temp) {
				printf(file_cach_err);
				return 1;
			}
			pspDebugScreenSetXY(0, 11);
			printf(inf_stat_deco);
		}
		j++;
		
		// check index
		index  = index_buf[block];
		plain  = index & 0x80000000;
		index  &= 0x7fffffff;
		if(plain)
			read_size = ciso.block_size;
		else
		{
			index2 = index_buf[block+1] & 0x7fffffff;
			read_size = (index2-index) << (ciso.align);
		}

		memcpy(block_buf2, prebufre, read_size);
		prebufre += read_size;
		z.avail_in  = read_size;

		if(plain)
		{
			memcpy(block_buf1,block_buf2,read_size);
			cmp_size = read_size;
		}
		else
		{
			z.next_out  = block_buf1;
			z.avail_out = ciso.block_size;
			z.next_in   = block_buf2;
			status = inflate(&z, Z_FULL_FLUSH);
			if (status != Z_STREAM_END)
			//if (status != Z_OK)
			{
				printf(infl_blck_err, block,(z.msg) ? z.msg : error_err_err,status);
				return 1;
			}
			cmp_size = ciso.block_size - z.avail_out;
			if(cmp_size != ciso.block_size)
			{
				printf(blck_size_err,block,cmp_size , ciso.block_size);
				return 1;
			}
		}
		memcpy(prebufwr, block_buf1, cmp_size);
		prebufwr+=cmp_size;
		total_size_offs_wr+=cmp_size;
		
		//write decompressed block
		if(j==numblocks_temp||block==ciso_total_block) {
			j=0;
			prebufwr=(unsigned char *)dir1;
			prebufre=(unsigned char *)dir2;
			pspDebugScreenSetXY(0, 11);
			printf(inf_stat_writ);
			if(sceIoWrite(fout, prebufwr, total_size_offs_wr) != total_size_offs_wr)
			{
				printf(file_writ_err);
				return 1;
			}
			total_size_offs_wr=0;
		}

		// term zlib
		if (inflateEnd(&z) != Z_OK)
		{
			printf(infl_end_err, (z.msg) ? z.msg : error_err_err);
			return 1;
		}
	}

	pspDebugScreenSetXY(0, 11);
	printf(dec_deco_comp);
	return 0;
}

/****************************************************************************
	compress ISO
****************************************************************************/
int comp_ciso(int level)
{
	unsigned long long file_size;
	unsigned long long write_pos;
	int dir1, dir2;
	int index_size;
	int block;
	unsigned char buf4[64];
	int cmp_size;
	int status;
	int percent_period;
	int percent_cnt;
	int align,align_b,align_m;
	int j=0;
	int total_size_offs_wr=0;
	int numblocks_temp=0;
	char inf_name_in[MAX_CHAR_FILENAME_INF], inf_name_out[MAX_CHAR_FILENAME_INF];

	file_size = check_file_size(&fin);
	if(file_size<0)
	{
		printf(file_size_err);
		return 1;
	}
	
	if(ciso_total_block<numblocks) numblocks_temp=ciso_total_block;
		else numblocks_temp=numblocks;

	// allocate index block
	index_size = (ciso_total_block + 1 ) * sizeof(unsigned long);
	index_buf  = malloc(index_size);
	crc_buf    = malloc(index_size);
	block_buf1 = malloc(ciso.block_size);
	block_buf2 = malloc(ciso.block_size*2);
	prebufwr = malloc((ciso.block_size+sizeof buf4)*(numblocks_temp+1));
	prebufre = malloc(ciso.block_size*(numblocks_temp+1));
	dir1 = (int)&prebufwr[0];
	dir2 = (int)&prebufre[0];

	if( !index_buf || !crc_buf || !block_buf1 || !block_buf2 || !prebufwr || !prebufre )
	{
		printf(mem_alloc_err);
		return 1;
	}
	memset(index_buf,0,index_size);
	memset(crc_buf,0,index_size);
	memset(buf4,0,sizeof(buf4));

	// init zlib
	z.zalloc = Z_NULL;
	z.zfree  = Z_NULL;
	z.opaque = Z_NULL;

	// show info
	getfilename(inf_name_in, fname_in);
	getfilename(inf_name_out, fname_out);
	printf(com_file_file,inf_name_in,inf_name_out);
	printf(inf_file_size,ciso.total_bytes);
	printf(inf_blck_size,ciso.block_size);
	printf(inf_indx_algn,1<<ciso.align);
	printf(com_com_level,level);

	// write header block
	sceIoWrite(fout, &ciso, sizeof(ciso));

	// dummy write index block
	sceIoWrite(fout, index_buf, index_size);

	write_pos = sizeof(ciso) + index_size;

	// compress data
	percent_period = ciso_total_block/100;
	percent_cnt    = ciso_total_block/100;

	align_b = 1<<(ciso.align);
	align_m = align_b -1;

	for(block = 0;block < ciso_total_block ; block++)
	{
		if(j==0) {
			if(numblocks_temp>(ciso_total_block-block)) numblocks_temp = (ciso_total_block-block);
			pspDebugScreenSetXY(0, 11);
			printf(inf_stat_cach);
			sceIoRead(fin, prebufre, ciso.block_size*numblocks_temp);
			pspDebugScreenSetXY(0, 11);
			printf(inf_stat_comp);
		}
		j++;
		
		if(--percent_cnt<=0)
		{	
			pspDebugScreenSetXY(0, 9);
			percent_cnt = percent_period;
			printf(com_com_avera
				,block / percent_period
				,block==0 ? 0 : 100*write_pos/(block*0x800));
		}

		if (deflateInit2(&z, level , Z_DEFLATED, -15,8,Z_DEFAULT_STRATEGY) != Z_OK)
		{
			printf(defl_init_err, (z.msg) ? z.msg : "???");
			return 1;
		}

		// write align
		align = (int)write_pos & align_m;
		if(align)
		{
			align = align_b - align;
			memcpy(prebufwr, buf4, align);
			prebufwr+=align;
			total_size_offs_wr+=align;
			write_pos += align;
		}

		// mark offset index
		index_buf[block] = write_pos>>(ciso.align);

		// read buffer
		z.next_out  = block_buf2;
		z.avail_out = ciso.block_size*2;
		z.next_in   = block_buf1;
		memcpy(block_buf1, prebufre, ciso.block_size);
		prebufre+=ciso.block_size;
		z.avail_in  = ciso.block_size;

		// compress block
//		status = deflate(&z, Z_FULL_FLUSH);
		status = deflate(&z, Z_FINISH);
		if (status != Z_STREAM_END)
//		if (status != Z_OK)
		{
			printf(defl_blck_err, block,(z.msg) ? z.msg : error_err_err,status);
			return 1;
		}

		cmp_size = ciso.block_size*2 - z.avail_out;

		// choise plain / compress
		if(cmp_size >= ciso.block_size)
		{
			cmp_size = ciso.block_size;
			memcpy(block_buf2,block_buf1,cmp_size);
			// plain block mark
			index_buf[block] |= 0x80000000;
		}

		// write compressed block
		memcpy(prebufwr, block_buf2, cmp_size);
		prebufwr+=cmp_size;
		total_size_offs_wr+=cmp_size;
		
		if(j==numblocks_temp || block == ciso_total_block) {
			j=0;
			prebufwr=(unsigned char *)dir1;
			prebufre=(unsigned char *)dir2;
			pspDebugScreenSetXY(0, 11);
			printf(inf_stat_writ);
			if(sceIoWrite(fout, prebufwr, total_size_offs_wr) != total_size_offs_wr)
			{
				printf(file_writ_err);
				return 1;
			}
			total_size_offs_wr=0;
		}

		// mark next index
		write_pos += cmp_size;

		// term zlib
		if (deflateEnd(&z) != Z_OK)
		{
			printf(defl_end_err, (z.msg) ? z.msg : error_err_err);
			return 1;
		}
	}

	// last position (total size)
	index_buf[block] = write_pos>>(ciso.align);

	// write header & index block
	pspDebugScreenSetXY(0, 11);
	printf(com_wr_header);
	sceIoLseek(fout,sizeof(ciso),SEEK_SET);
	sceIoWrite(fout, index_buf, index_size);

	pspDebugScreenSetXY(0, 11);
	printf(com_comp_comp
		,(int)write_pos,(int)(write_pos*100/ciso.total_bytes));
	return 0;
}

/****************************************************************************
	Main
****************************************************************************/
int main()
{
	int whattodo=0;

	pspDebugScreenInit();
	scePowerSetClockFrequency(333, 333, 166);
	
	while(1) {
	
		pspDebugScreenSetBackColor(0xFF0000);
		pspDebugScreenSetTextColor(0x00FFFF);
		pspDebugScreenClear();
	
		fname_in = file_browser("ms0:/ISO/");
		
#ifdef DEBUG_FILENAME
		printf("\nAntes: fname_in = %s.\n", fname_in);
#endif
	
		if(!fname_in)
			sceKernelExitGame();
		else {
			if(stricmp(strrchr(fname_in, '.'), ".cso") == 0) {
				strcpy(fname_out, fname_in);
				fname_out[strlen(fname_out)-3] = 'i';
				whattodo=0;
			}
			if(stricmp(strrchr(fname_in, '.'), ".iso") == 0) {
				strcpy(fname_out, fname_in);
				fname_out[strlen(fname_out)-3] = 'c';
				whattodo=1;
			}
		}
		
#ifdef DEBUG_FILENAME
		printf("Despues: fname_in = %s.\nfname_out = %s.\nwhattodo = %i.", fname_in, fname_out, whattodo);
		while(1) sceDisplayWaitVblankStart();
#endif

		const char *constfname_in=fname_in;
		const char *constfname_out=fname_out;
	
		pspDebugScreenClear();
		pspDebugScreenSetXY(0, 0);
		pspDebugScreenSetTextColor(0xFFFFFF);
		printf("Ciso converter port/mod by BlackSith, originally created by Booster\n\n");
		pspDebugScreenSetTextColor(0x00FFFF);

		if ((int*)(fin = sceIoOpen(constfname_in, PSP_O_RDONLY, 0777)) == NULL)
		{
			printf(file_open_err, fname_in);
			return 1;
		}
		if ((int*)(fout = sceIoOpen(constfname_out, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777)) == NULL)
		{
			printf(file_crea_err, fname_out);
			return 1;
		}

		if(!whattodo)
			decomp_ciso(fin, fout);
		else
			comp_ciso(level);

		sceKernelDelayThread(2400000);

		// free memory
		freemem();

		// close files
		pspDebugScreenSetXY(0, 11);
		printf(inf_stat_clos);
		sceIoClose(fin);
		sceIoClose(fout);
		sceKernelDelayThread(2000000);
	
	}

	sceKernelExitGame();

	return 0;
}

