
#include "conf.h"

extern int level;
extern int numblocks;

struct dirent {
		u32 type;
		char name[256];
};
static int nfiles = 0;
static struct dirent files[MAX_ENTRY];
static SceIoDirent dir;
static SceCtrlData pad;
char curr_dir[MAX_PATH];

/****************************************************************************
	Show Info
****************************************************************************/
void showinfo(void) {
	
	int y=(272/8)-6;
	
	pspDebugScreenSetTextColor(0xFFFFFF);
	
	pspDebugScreenSetXY(0, y);
	printf(inf_keys_fun1);
	pspDebugScreenSetXY(0, ++y);
	printf(inf_keys_fun2);
	pspDebugScreenSetXY(0, ++y);
	if(numblocks==1) printf(inf_keys_fun3a);
	else printf(inf_keys_fun3b, numblocks);
	pspDebugScreenSetXY(0, ++y);
	printf(inf_keys_fun4);
	pspDebugScreenSetXY(0, ++y);
	printf(inf_keys_fun5);
	pspDebugScreenSetXY(0, ++y);
	printf(inf_keys_fun6, level);
}

/*--------------------------------------------------------
	Compare file name
 -------------------------------------------------------*/

int cmpFile(struct dirent *a, struct dirent *b)
{
	char ca, cb;
	int i, n, ret;

	if (a->type == b->type)
	{
		n = strlen(a->name);

		for (i = 0; i <= n; i++)
		{
			ca = a->name[i];
			cb = b->name[i];
			if (ca >= 'a' && ca <= 'z') ca -= 0x20;
			if (cb >= 'a' && cb <= 'z') cb -= 0x20;

			ret = ca - cb;
			if (ret) return ret;
		}
		return 0;
	}

	return  (a->type & FIO_SO_IFDIR) ? -1 : 1;
}

/*--------------------------------------------------------
	Quick sort
 -------------------------------------------------------*/

void sort(struct dirent *a, int left, int right)
{
	struct dirent tmp, pivot;
	int i, p;

	if (left < right)
	{
		pivot = a[left];
		p = left;

		for (i = left + 1; i <= right; i++)
		{
			if (cmpFile(&a[i],&pivot) < 0)
			{
				p++;
				tmp = a[p];
				a[p] = a[i];
				a[i] = tmp;
			}
		}

		a[left] = a[p];
		a[p] = pivot;

		sort(a, left, p - 1);
		sort(a, p + 1, right);
	}
}

/*--------------------------------------------------------
	Get directory entry
 -------------------------------------------------------*/
static void getDir(const char *path)
{
	static int fd;
	int b = 0;

	memset(&dir, 0, sizeof(dir));

	nfiles = 0;

	if (strcmp(path, "ms0:/") != 0)
	{
		strcpy(files[nfiles].name, "..");
		files[nfiles].type = FIO_SO_IFDIR;
		nfiles++;
		b = 1;
	}

	fd = sceIoDopen(path);

	while (nfiles < MAX_ENTRY)
	{
		char *ext;
		static SceIoDirent dir = { {0} };

		if (sceIoDread(fd, &dir) <= 0) break;

		if (dir.d_name[0] == '.') continue;

		if ((ext = strrchr(dir.d_name, '.')) != NULL)
		{
			if (stricmp(ext, ".cso") == 0)
			{
				strcpy(files[nfiles].name, dir.d_name);
				files[nfiles].type = 0;
				nfiles++;
				continue;
			}
			if (stricmp(ext, ".iso") == 0)
			{
				strcpy(files[nfiles].name, dir.d_name);
				files[nfiles].type = 0;
				nfiles++;
				continue;
			}
		}

		if (dir.d_stat.st_attr == FIO_SO_IFDIR)
		{
			strcpy(files[nfiles].name, dir.d_name);
			strcat(files[nfiles].name, "/");
			files[nfiles].type = FIO_SO_IFDIR;
			nfiles++;
		}
	}

	sceIoDclose(fd);

	if (b)
		sort(files + 1, 0, nfiles - 2);
	else
		sort(files, 0, nfiles - 1);
}

void modify_display_path(char *path, char *org_path, int max_width)
{
	if (strlen(org_path) > max_width)
	{
		int i, j, num_dir = 0;
		char temp[MAX_PATH], *dir[256];

		strcpy(temp, org_path);

		dir[num_dir++] = strtok(temp, "/");

		while ((dir[num_dir] = strtok(NULL, "/")) != NULL) {
			num_dir++;
		}

		j = num_dir - 1;

		do
		{
			j--;

			path[0] = '\0';

			for (i = 0; i < j; i++)
			{
				strcat(path, dir[i]);
				strcat(path, "/");
			}

			strcat(path, "[...]/");
			strcat(path, dir[num_dir - 1]);
			strcat(path, "/");

		} while (strlen(path) > max_width);
	}
	else strcpy(path, org_path);
}

/*--------------------------------------------------------
	Run filer
 -------------------------------------------------------*/

char *file_browser(const char *arg)
{	
	int i, sel = 0, rows = (272/8)-12, top = 0;
	int update = 1, prev_sel = 0, exit = 0, key_down = 0;
	char *p;

	memset(curr_dir, 0, sizeof(curr_dir));
	strncpy(curr_dir, arg, sizeof(curr_dir) - 1);

	memset(files, 0, sizeof(files));

	getDir(curr_dir);

	pspDebugScreenClear();

	while (!exit)
	{
		if (update)
		{
			char path[MAX_PATH];
			char name[MAX_PATH];
			
			sceDisplayWaitVblankStart();
			pspDebugScreenClear();

			pspDebugScreenSetXY(0, 0);
			pspDebugScreenSetTextColor(0xFFFFFF);
			printf("Ciso converter port/mod by BlackSith, originally created by Booster\n\n");
			showinfo();
			modify_display_path(path, curr_dir, 480/5);
			pspDebugScreenSetXY(2, 2);
			printf(path);

			for (i = 0; i < rows; i++)
			{
				if (top + i >= nfiles) break;

				if (top + i == sel)
				{
					pspDebugScreenSetXY(2, 4 + i);
					pspDebugScreenSetTextColor(0x000000);
					modify_display_path(name, files[sel].name, 480/5);
					printf("[%s]", name);
				}
				else
				{
					pspDebugScreenSetXY(3, 4 + i);
					pspDebugScreenSetTextColor(0x00FFFF);
					modify_display_path(name, files[top + i].name, 480/5);
					printf(name);
				}
			}

			update = 0;
		}

		prev_sel = sel;
		
		sceCtrlReadBufferPositive((SceCtrlData *)(&pad), 1);

		if(!key_down) {
		if (pad.Buttons&(PSP_CTRL_UP))
		{
			sel--;
		}
		else if (pad.Buttons&(PSP_CTRL_DOWN))
		{
			sel++;
		}
		else if (pad.Buttons&(PSP_CTRL_LEFT))
		{
			sel -= rows;
			if (sel < 0) sel = 0;
		}
		else if (pad.Buttons&(PSP_CTRL_RIGHT))
		{
			sel += rows;
			if (sel >= nfiles) sel = nfiles - 1;
		}
		else if (pad.Buttons&(PSP_CTRL_CROSS))
		{
			if (files[sel].type == FIO_SO_IFDIR)
			{
				if (!strcmp(files[sel].name, ".."))
				{
					if (strcmp(curr_dir, "ms0:/") != 0)
					{
						char old_dir[MAX_PATH];

						*strrchr(curr_dir, '/') = '\0';
						p = strrchr(curr_dir, '/') + 1;
						strcpy(old_dir, p);
						strcat(old_dir,  "/");
						*p = '\0';

						getDir(curr_dir);
						sel = 0;
						prev_sel = -1;

						for (i = 0; i < nfiles; i++)
						{
							if (!strcmp(old_dir, files[i].name))
							{
								sel = i;
								top = sel - 3;
								break;
							}
						}
					}
				}
	   			else
	   			{
					strcat(curr_dir, files[sel].name);
					getDir(curr_dir);
				}
				sel=0;
			}
			else {
				strcat(curr_dir, files[sel].name);
				getDir(curr_dir);
				return (char *)&curr_dir;
			}
			update=1;
			
		}
		else if (pad.Buttons&(PSP_CTRL_TRIANGLE))
		{
			exit = 1;
			break;
			update = 1;
		}
		else if (pad.Buttons&(PSP_CTRL_CIRCLE))
		{
			if (strcmp(curr_dir, "ms0:/") != 0) {
				char old_dir[MAX_PATH];
				*strrchr(curr_dir, '/') = '\0';
				p = strrchr(curr_dir, '/') + 1;
				strcpy(old_dir, p);
				strcat(old_dir,  "/");
				*p = '\0';
				getDir(curr_dir);
				sel = 0;
				prev_sel = -1;
			}
			
			update = 1;
		}
		else if (pad.Buttons&(PSP_CTRL_LTRIGGER))
		{
			numblocks-=1000;
			if(numblocks<=0) numblocks = 1;
			update = 1;
		}
		else if (pad.Buttons&(PSP_CTRL_RTRIGGER))
		{
			numblocks+=1000;
			if(numblocks%10) numblocks--;
			if(numblocks>5000) numblocks = 5000;
			update = 1;
		}
		key_down=1;
		}
		
		if(!(pad.Buttons&(PSP_CTRL_RTRIGGER|PSP_CTRL_LTRIGGER|PSP_CTRL_CIRCLE|PSP_CTRL_TRIANGLE|PSP_CTRL_CROSS|PSP_CTRL_SQUARE
		|PSP_CTRL_START|PSP_CTRL_SELECT|PSP_CTRL_UP|PSP_CTRL_RIGHT|PSP_CTRL_DOWN|PSP_CTRL_LEFT)))
			key_down=0;

		if (top > nfiles - rows) top = nfiles - rows;
		if (top < 0) top = 0;
		if (sel >= nfiles) sel = 0;
		if (sel < 0) sel = nfiles - 1;
		if (sel >= top + rows) top = sel - rows + 1;
		if (sel < top) top = sel;

		if (prev_sel != sel)
		{
			update = 1;
		}
	}
	
	return NULL;
}
