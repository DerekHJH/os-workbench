#include <common.h>
extern cpu_t cpuinfo[];
file_t *ofile[NOFILE];

static int fdalloc(file_t *f)
{
	int fd;
	for(fd = 0; fd < NOFILE; fd++)
	{
    if(ofile[fd] == 0)
		{
      ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

static int vfs_write(int fd, void *buf, int count)
{
	return filewrite(ofile[fd], buf, count);
}
static int vfs_read(int fd, void *buf, int count)
{
	return fileread(ofile[fd], buf, count);
}
static int vfs_close(int fd)
{
	file_t *f = ofile[fd];
	ofile[fd] = NULL;
	fileclose(f);
	return 0;
}
static inode_t *create(char *path, short type, short major, short minor)
{
  inode_t *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)return 0;
  ilock(dp);
	printf("dev->size %d\n", dp->size);

  if((ip = dirlookup(dp, name, 0)) != 0)
	{
    iunlockput(dp);
    ilock(ip);
    if(type == T_FILE && ip->type == T_FILE)return ip;
    iunlockput(ip);
    return 0;
  }

  panic_on((ip = ialloc(dp->dev, type)) == 0, "\033[31m create: ialloc\n \033[0m");

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if(type == T_DIR)
	{  // Create . and .. entries.
    dp->nlink++;  // for ".."
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    panic_on(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0, "\033[31m create dots\n \033[0m");
  }

  panic_on(dirlink(dp, name, ip->inum) < 0, "\033[31m create: dirlink\n \033[0m");

  iunlockput(dp);

  return ip;
}
static int vfs_open(const char *pathname, int flags)
{
  int fd;
  file_t *f;
  inode_t *ip;

  if(flags & O_CREAT)
	{
    ip = create((char *)pathname, T_FILE, 0, 0);
    if(ip == 0)return -1;
  } 
	else 
	{
    if((ip = namei((char *)pathname)) == 0)return -1;
    ilock(ip);
		/*
    if(ip->type == T_DIR && flags != O_RDONLY)
		{
      iunlockput(ip);
      return -1;
    }
		*/
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0)
	{
    if(f)fileclose(f);
    iunlockput(ip);
    return -1;
  }
  iunlock(ip);

  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0;
  f->readable = !(flags & O_WRONLY);
  f->writable = (flags & O_WRONLY) || (flags & O_RDWR);
  return fd;
}
static int vfs_lseek(int fd, int offset, int whence)
{
	switch(whence)
	{
		case SEEK_SET:
		{
			ofile[fd]->off = offset;
			break;
		}
		case SEEK_CUR:
    {
      ofile[fd]->off += offset;
    	break;
    }
		case SEEK_END:
    {
      ofile[fd]->off = ofile[fd]->ip->size + offset;
    	break;
    }
		default: return -1;
	}
	return ofile[fd]->off;
}
static int vfs_link(const char *oldpath, const char *newpath)
{
	char name[DIRSIZ];
  inode_t *dp, *ip;

	if((ip = namei((char *)oldpath)) == 0)return -1;


  ilock(ip);
  if(ip->type == T_DIR)
	{
    iunlockput(ip);
    return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if((dp = nameiparent((char *)newpath, name)) == 0)goto bad;

  ilock(dp);
  if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0)
	{
    iunlockput(dp);
    goto bad;
  }
  iunlockput(dp);
  iput(ip);

	return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  return -1;
}
static int isdirempty(inode_t *dp)
{
  int off;
  dirent_t de;

  for(off = 2 * sizeof(de); off < dp->size; off += sizeof(de))
	{
    panic_on(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de), "\033[31m isdirempty: readi\n \033[0m");
    if(de.inode != 0)return 0;
  }
  return 1;
}
static int vfs_unlink(const char *pathname)
{
	inode_t *ip, *dp;
  dirent_t de;
  char name[DIRSIZ];
  uint32_t off;


  if((dp = nameiparent((char *)pathname, name)) == 0)return -1;

  ilock(dp);

  // Cannot unlink "." or "..".
  if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)goto bad;

  if((ip = dirlookup(dp, name, &off)) == 0)goto bad;

  ilock(ip);

  panic_on(ip->nlink < 1, "\033[31m unlink: nlink < 1\n \033[0m");
  if(ip->type == T_DIR && !isdirempty(ip))
	{
    iunlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  panic_on(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de), "\033[31m unlink: writei\n \033[0m");
  if(ip->type == T_DIR)
	{
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  return 0;

bad:
  iunlockput(dp);
  return -1;
}
static int vfs_fstat(int fd, struct ufs_stat *buf)
{
	return filestat(ofile[fd], (stat_t *)buf);
}
static int vfs_mkdir(const char *pathname)
{
  inode_t *ip;
  if((ip = create((char *)pathname, T_DIR, 0, 0)) == 0)return -1;
  iunlockput(ip);
	return 0;
}
static int vfs_chdir(const char *path)
{
  inode_t *ip;
  task_t *curtask = cpuinfo[_cpu()].current;
  
  if((ip = namei((char *)path)) == 0)return -1;

  ilock(ip);
  if(ip->type != T_DIR)
	{
    iunlockput(ip);
    return -1;
  }
  iunlock(ip);
  iput(curtask->cwd);
  curtask->cwd = ip;
	return 0;
}

static int vfs_dup(int fd)
{
	int fd2;
	if((fd2 = fdalloc(ofile[fd])) < 0)return -1;
	filedup(ofile[fd]);
	return fd2;
}
static void vfs_init()
{
	binit();
	iinit();
	fileinit();
	char dirname[128], filename[128], content[128];
	int fd = 0;
	for(int i = 0; i < _ncpu(); i++)
	{
		cpuinfo[i].idle.cwd = namei("/");
		sprintf(dirname, "/proc/%d", cpuinfo[i].idle.id);
		sprintf(filename, "%s/name", dirname);
		sprintf(content, "%s", cpuinfo[i].idle.name);
		vfs_mkdir(dirname);
		panic_on((fd = vfs_open(filename, O_CREAT | O_RDWR)) < 0, "\033[31m in vfs_init file open failed ! \n\033[0m");
		vfs_write(fd, content, sizeof(content));
		vfs_close(fd);	
	}	
	return;
}


void fileoperation(void *arg)
{
	/*==============necessary operation========*/
	
	inode_t *ip = create("/dev/zero", T_DEV, ZERO, 0);
	iunlockput(ip);
	/*
	ip = create("/dev/null", T_DEV, NUL, 0);
	iunlockput(ip);
	ip = create("/dev/random", T_DEV, RANDOM, 0);
	iunlockput(ip);
  */


	/*===========necessary operation===========*/

	/*
	char *filename = (char *)arg;

	printf("filename is %s\n", filename);
	printf("before open\n");
	int fd = vfs->open(filename, O_CREAT | O_RDWR);
	printf("after open\n");
	if(fd < 0)
	{
		printf("shit fd is negative\n");
	}
	printf("before write\n");
	vfs->write(fd, filename, strlen(filename));
	printf("after write\n");
	char ans[128] = "\0";
	printf("before lseek\n");
  vfs->lseek(fd, 0, SEEK_SET);
  printf("after lseek\n");


	printf("before read\n");
	vfs->read(fd, ans, strlen(filename));
	printf("after read\n");
	printf("===================ans is %s\n", ans);

	printf("====================================================\n\n");
	
	char fname[128];
	sprintf(fname, "/dev/zero");
  fd = vfs->open(fname, O_CREAT | O_RDWR);
	if(fd < 0)
  {
  	printf("shit fd is negative\n");
  }
	printf("before read\n"); 
	vfs->read(fd, ans, 3);
	ans[3] = 0;
  printf("after read\n");
	printf("=============ans is %s\n", ans);
	*/
	while(1);
}

MODULE_DEF(vfs) = 
{
	.init = vfs_init,
  .write = vfs_write,
  .read = vfs_read,
  .close = vfs_close,
  .open = vfs_open,
  .lseek = vfs_lseek,
  .link = vfs_link,
  .unlink = vfs_unlink,
  .fstat = vfs_fstat,
  .mkdir = vfs_mkdir,
  .chdir = vfs_chdir,
  .dup = vfs_dup,
};

