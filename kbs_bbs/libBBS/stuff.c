/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#define EXTERN

#include "bbs.h"
#include <sys/param.h>
#include <sys/ipc.h>
#include <sys/shm.h>

extern char *getenv();
static const char *invalid[] = { "unknown@", "root@", "gopher@", "bbs@",
        "guest@", NULL };
static char modestr[STRLEN];
static char hh_mm_ss[8];

int my_system(const char *cmdstring)
{
        pid_t pid;
        int status;
        if (!cmdstring) return 1;
        switch (pid = fork())
        {
                case -1:
                   status = -1;
                   break;
                case 0:
                   {
                        int fd = open("/dev/zero",O_RDWR);
                        dup2(fd,2);
                        dup2(fd,1);
                        if (fd !=1 && fd !=2) close(fd);
                        execl("/bin/sh","sh","-c",cmdstring,NULL);
                        _exit(127);
                   }
                default:
                        while (waitpid(pid,&status,0)<0)
                            if (errno!=EINTR) {
                                status = -1;
                                break;
                        }
                break;
        }
        return status;
}

char *idle_str( struct user_info *uent )
{
    time_t      now, diff;
    int         hh, mm;

    now = time(0);
    diff = now - uent->freshtime;
    if (diff==now) /* @#$#!@$#@! */
        diff=0;
#ifdef DOTIMEOUT
    /* the 60 * 60 * 24 * 5 is to prevent fault /dev mount from
       kicking out all users */

    if ((diff > IDLE_TIMEOUT) && (diff < 60 * 60 * 24 * 5 ))
        kill( uent->pid, SIGHUP );
#endif

    hh = diff / 3600;
    mm = (diff / 60) % 60;

    if ( hh > 0 )
        snprintf( hh_mm_ss,sizeof(hh_mm_ss), "%2d:%02d", hh, mm );
    else if ( mm > 0 )
        sprintf( hh_mm_ss, "%d", mm );
    else sprintf ( hh_mm_ss, "   ");


    return hh_mm_ss;
}

char *modestring(int mode,int towho,int complete,char *chatid)
{
    struct userec urec;

    /* Leeward: 97.12.18: Below removing ' characters for more display width */
    if (chatid) {
        if (complete) sprintf(modestr, "%s %s", ModeType(mode), chatid);
        else return ((char*)ModeType(mode));
        return (modestr);
    }
    if (mode != TALK && mode != PAGE && mode != QUERY)
        return ((char*)ModeType(mode));
    /*---	modified by period	2000-10-20	---*
        if (get_record(PASSFILE, &urec, sizeof(urec), towho) == -1)
            return (ModeType(mode));
    ---*/
    if (complete) {
	    if(getuserid(urec.userid, towho) != towho) return (char*)ModeType(mode);
        sprintf(modestr, "%s %s", ModeType(mode), urec.userid);
    }
    else
        return ((char*)ModeType(mode));
    return (modestr);
}

int
dashf( fname )
char *fname;
{
    struct stat st;

    return ( stat( fname, &st ) == 0 && S_ISREG( st.st_mode ) );
}

int
dashd( fname )
char    *fname;
{
    struct stat st;

    return ( stat( fname, &st ) == 0 && S_ISDIR( st.st_mode ) );
}

/* rrr - Snagged from pbbs 1.8 */

#define LOOKFIRST  (0)
#define LOOKLAST   (1)
#define QUOTEMODE  (2)
#define MAXCOMSZ (1024)
#define MAXARGS (40)
#define MAXENVS (20)
#define BINDIR "/bin/"

char *bbsenv[MAXENVS] ;
int numbbsenvs = 0 ;

/* Case Independent strncmp */

int
ci_strncmp(s1,s2,n)
register char *s1,*s2 ;
register int n ;
{
    char        c1, c2;

    while( n-- > 0 ) {
        c1 = *s1++;
        c2 = *s2++;
        if( c1 >= 'a' && c1 <= 'z' )
            c1 &= 0xdf;
        if( c2 >= 'a' && c2 <= 'z' )
            c2 &= 0xdf;
        if( c1 != c2 )
            return (c1 - c2);
        if( c1 == 0 )
            return 0;
    }
    return 0;
}

int
ci_strcmp( s1, s2 )
register char   *s1, *s2;
{
    char        c1, c2;

    while( 1 ) {
        c1 = *s1++;
        c2 = *s2++;
        if( c1 >= 'a' && c1 <= 'z' )
            c1 &= 0xdf;
        if( c2 >= 'a' && c2 <= 'z' )
            c2 &= 0xdf;
        if( c1 != c2 )
            return (c1 - c2);
        if( c1 == 0 )
            return 0;
    }
}

int
bbssetenv(env,val)
char *env, *val ;
{
    register int i,len ;

    if(numbbsenvs == 0)
        bbsenv[0] = NULL ;
    len = strlen(env) ;
    for(i=0;bbsenv[i];i++)
        if(!strncasecmp(env,bbsenv[i],len))
            break ;
    if(i>=MAXENVS)
        return -1 ;
    if(bbsenv[i])
        free(bbsenv[i]) ;
    else
        bbsenv[++numbbsenvs] = NULL ;
    bbsenv[i] = malloc(strlen(env)+strlen(val)+2) ;
    strcpy(bbsenv[i],env) ;
    strcat(bbsenv[i],"=") ;
    strcat(bbsenv[i],val) ;
    return 0;
}

#ifdef BBSMAIN
int
do_exec(com,wd)
char *com, *wd ;
{
    char path[MAXPATHLEN] ;
    char pcom[MAXCOMSZ] ;
    char *arglist[MAXARGS] ;
    char *tz;
    register int i,len ;
    register int argptr ;
    int status, pid, w ;
    int pmode ;
    void (*isig)(), (*qsig)() ;

    strncpy(path,BINDIR,MAXPATHLEN) ;
    strncpy(pcom,com,MAXCOMSZ) ;
    len = Min(strlen(com)+1,MAXCOMSZ) ;
    pmode = LOOKFIRST ;
    for(i=0,argptr=0;i<len;i++) {
        if(pcom[i] == '\0')
            break ;
        if(pmode == QUOTEMODE) {
            if(pcom[i] == '\001') {
                pmode = LOOKFIRST ;
                pcom[i] = '\0' ;
                continue ;
            }
            continue ;
        }
        if(pcom[i] == '\001') {
            pmode = QUOTEMODE ;
            arglist[argptr++] = &pcom[i+1] ;
            if(argptr+1 == MAXARGS)
                break ;
            continue ;
        }
        if(pmode == LOOKFIRST) {
            if(pcom[i] != ' ') {
                arglist[argptr++] = &pcom[i] ;
                if(argptr+1 == MAXARGS)
                    break ;
                pmode = LOOKLAST ;
            } else continue ;
        }
        if(pcom[i] == ' ') {
            pmode = LOOKFIRST ;
            pcom[i] = '\0' ;
        }
    }
    arglist[argptr] = NULL ;
    if(argptr == 0)
        return -1 ;
    if(*arglist[0] == '/')
        strncpy(path,arglist[0],MAXPATHLEN) ;
    else
        strncat(path,arglist[0],MAXPATHLEN) ;
#ifdef IRIX
    if((pid = fork()) == 0) {
#else
if((pid = vfork()) == 0) {
#endif
        if(wd)
            if(chdir(wd)) {
                /* change by KCN 1999.09.08
                              fprintf(stderr,"Unable to chdir to '%s'\n",wd) ;
                */
                prints("Unable to chdir to '%s'\n",wd) ;
                oflush();
                exit(-1) ;
            }
        bbssetenv("PATH", "/bin:.");
        bbssetenv("TERM", "vt100");
        bbssetenv("USER", currentuser->userid);
        bbssetenv("USERNAME", currentuser->username);
        /* added for tin's reply to */
        bbssetenv("REPLYTO", currentuser->email);
        bbssetenv("FROMHOST", fromhost);
        /* end of insertion */
        if ((tz = getenv("TZ")) != NULL)
            bbssetenv("TZ", tz);
        if(numbbsenvs == 0)
            bbsenv[0] = NULL ;
        execve(path,arglist,bbsenv) ;
        /* change by KCN 1999.09.08
                fprintf(stderr,"EXECV FAILED... path = '%s'\n",path) ;
        	*/
        prints("EXECV FAILED... path = '%s'\n",path) ;
        oflush();
        exit(-1) ;
    }
    isig = signal(SIGINT, SIG_IGN) ;
    qsig = signal(SIGQUIT, SIG_IGN) ;
    while((w = wait(&status)) != pid && w != 1)
        /* NULL STATEMENT */ ;
    signal(SIGINT, isig) ;
    signal(SIGQUIT, qsig) ;
    return((w == -1)? w: status) ;
}
#endif

#ifdef kill
#undef kill
#endif

int safe_kill(int x, int y)
{  if(x<=0) return -1;
   return kill(x,y);
}



char * Cdate(time_t  clock) /* 时间 --> 英文 */
{
    /* Leeward 2000.01.01 Adjust year display for 20** */
    static char foo[24/*22*/];
    struct tm *mytm = localtime(&clock);

    strftime(foo, 24/*22*/, "%Y-%m-%d %T %a"/*"%D %T %a"*/, mytm);
    return (foo);
}

char * Ctime(time_t  clock) /* 时间 转换 成 英文 */
{
    char *foo;
    char *ptr = ctime(&clock);

    if ((foo = strchr(ptr, '\n'))!=NULL) *foo = '\0';
    return (ptr);
}

int Isspace(char ch)
{
    return (ch == ' ' || ch =='\t' || ch == 10 || ch == 13);
}


char * nextword(const char * * str, char *buf, int sz)
{
    while (Isspace(**str))(*str)++;
    
    while (**str && !Isspace(**str)){
        if(sz>0){
            if(sz>1)  *buf++=**str;
            sz--;
        }
        (*str)++;
    }
    *buf=0;
    while (Isspace(**str))(*str)++;
    return buf;
}

#ifndef BBSMAIN
void
attach_err( shmkey, name )
int     shmkey;
char    *name;
{
    bbslog( "3system", "Attach:Error! %s error! key = %x.msg:%s\n", name, shmkey ,strerror(errno));
    exit( 1 );
}

void *attach_shm( char    *shmstr,int     defaultkey, int shmsize,int* iscreate)
{
	return attach_shm1(shmstr,defaultkey, shmsize,iscreate,0,NULL);
}

void *attach_shm1( char    *shmstr,int     defaultkey, int shmsize,int* iscreate,int readonly, void* shmaddr)
{
    void        *shmptr;
    int         shmkey=0, shmid;
 
    shmkey = defaultkey;
    shmid = shmget( shmkey, shmsize, 0 );
    if( shmid < 0 ) {
        if (readonly) {
            attach_err( shmkey, "shmget:readonly" );
            return 0;
        }
        shmid = shmget( shmkey, shmsize, IPC_CREAT | 0600 );
        if( shmid < 0 )
            attach_err( shmkey, "shmget" );
        shmptr = (void *) shmat( shmid, shmaddr, 0 );
        if( shmptr == (void *)-1 )
            attach_err( shmkey, "shmat" );
	else
            memset( shmptr, 0, shmsize );
        *iscreate=1;
    } else {
        if (readonly)
            shmptr = (void *) shmat( shmid, shmaddr, SHM_RDONLY);
        else
            shmptr = (void *) shmat( shmid, shmaddr, 0 );
        if( shmptr == (void *)-1 )
            attach_err( shmkey, "shmat" );
        *iscreate=0;
    }
    return shmptr;
}
#else
void attach_err( int     shmkey,char    *name)
{
    prints( "Error! %s error! key = %x.\n", name, shmkey );
    oflush();
    exit( 1 );
}

void *attach_shm( char    *shmstr,int     defaultkey, int shmsize,int* iscreate)
{
	return attach_shm1(shmstr,defaultkey, shmsize,iscreate,0 , NULL);
}

void *attach_shm1( char    *shmstr,int     defaultkey, int shmsize,int* iscreate,int readonly, void* shmaddr)
{
    void        *shmptr;
    int         shmkey, shmid;

    if (shmstr)
    	shmkey = sysconf_eval( shmstr );
    else shmkey=0;
    if( shmkey < 1024 )
        shmkey = defaultkey;
    shmid = shmget( shmkey, shmsize, 0 );
    if( shmid < 0 ) {
        if (readonly) {
            attach_err( shmkey, "shmget:readonly" );
            return 0;
        }
        shmid = shmget( shmkey, shmsize, IPC_CREAT | 0660 ); /* modified by dong , for web application , 1998.12.1 */
        *iscreate=YEA;
        if( shmid < 0 ) {
            attach_err( shmkey, "shmget" );
            exit(0);
        }
        shmptr = (void *) shmat( shmid, shmaddr, 0 );
        if( shmptr == (void *)-1 ) {
            attach_err( shmkey, "shmat" );
            exit(0);
    	} else
            memset( shmptr, 0, shmsize );
    } else {
    	*iscreate=0;
        if (readonly)
            shmptr = (void *) shmat( shmid, shmaddr, SHM_RDONLY);
        else
            shmptr = (void *) shmat( shmid, shmaddr, 0 );
        if( shmptr == (void *)-1 ) {
            attach_err( shmkey, "shmat" );
            exit(0);
        }
    }
    return shmptr;
}
#endif

/*
char *
cexp(exp)
int exp;
{
        int expbase=0;

        if(exp==-9999)
                return "没等级";
        if(exp<=100+expbase)
                return "新手上路";
        if(exp>100+expbase&&exp<=450+expbase)
                return "一般站友";
        if(exp>450+expbase&&exp<=850+expbase)
                return "中级站友";
        if(exp>850+expbase&&exp<=1500+expbase)
                return "高级站友";
        if(exp>1500+expbase&&exp<=2500+expbase)
                return "老站友";
        if(exp>2500+expbase&&exp<=3000+expbase)
                return "长老级";
        if(exp>3000+expbase&&exp<=5000+expbase)
                return "本站元老";
        if(exp>5000+expbase)
                return "开国大老";
        
}

char *
cperf(perf)
int perf;
{        
        
        if(perf==-9999)
                return "没等级";
        if(perf<=5)
                return "赶快加油";
        if(perf>5&&perf<=12)
                return "努力中";
        if(perf>12&&perf<=35)
                return "还不错";
        if(perf>35&&perf<=50)
                return "很好";
        if(perf>50&&perf<=90)
                return "优等生";
        if(perf>90&&perf<=140)
                return "太优秀了";
        if(perf>140&&perf<=200)
                return "本站支柱";
        if(perf>200)
                return "神～～";

}
*/
int countexp(struct userec *udata)
{
    int exp;

    if(!strcmp(udata->userid,"guest"))
        return -9999;
    exp=udata->numposts +/*post_in_tin( udata->userid )*/+udata->numlogins/5+(time(0)-udata->firstlogin)/86400+udata->stay/3600;
    return exp>0?exp:0;
}

int countperf(struct userec *udata)
{
    int perf;
    int reg_days;

    if(!strcmp(udata->userid,"guest"))
        return -9999;
    reg_days=(time(0)-udata->firstlogin)/86400+1;
    perf=((float)(udata->numposts/*+post_in_tin( udata->userid )*/)/(float)udata->numlogins+
          (float)udata->numlogins/(float)reg_days)*10;
    return perf>0?perf:0;
}


int compute_user_value( struct userec *urec)
{
    int         value;
    int        registeryear;
    int        basiclife;
    /* if (urec) has CHATCLOAK permission, don't kick it */
	/* 元老和荣誉帐号 在不自杀的情况下， 生命力999 Bigman 2001.6.23 */
    /* 
    * zixia 2001-11-20 所有的生命力都使用宏替换，
    * 在 smth.h/zixia.h 中定义 
    * */
    /* 特殊处理请移动出cvs 代码*/
    
#if 0
#ifdef ZIXIA
    if( urec->userlevel & PERM_MM )
	return LIFE_DAY_SYSOP;
#endif
#endif 

    if( (urec->userlevel & PERM_HORNOR)  && !(urec->userlevel & PERM_LOGINOK))
        return LIFE_DAY_LONG;

    
    if( ((urec->userlevel & PERM_HORNOR)||(urec->userlevel & PERM_CHATCLOAK )) && (!(urec->userlevel & PERM_SUICIDE)))
        return LIFE_DAY_NODIE;

    if ( urec->userlevel & PERM_SYSOP) 
	return LIFE_DAY_SYSOP;
	/* 站务人员生命力不变 Bigman 2001.6.23 */
	
   
    value = (time(0) - urec->lastlogin) / 60;    /* min */
    if (0 == value) value = 1; /* Leeward 98.03.30 */

  /* 去掉长期帐号之类 */
#if 0
 
    /* 修改: 将永久帐号转为长期帐号, Bigman 2000.8.11 */
    if ((urec->userlevel & PERM_XEMPT) && (!(urec->userlevel & PERM_SUICIDE)) )
    {	if (urec->lastlogin < 988610030)
        return LIFE_DAY_LONG; /* 如果没有登录过的 */
        else
            return (LIFE_DAY_LONG * 24 * 60 - value)/(60*24);
    }
    
 #endif 

   
 
    /* new user should register in 30 mins */
    if( strcmp( urec->userid, "new" ) == 0 ) {
        return (LIFE_DAY_NEW - value) / 60; /* *->/ modified by dong, 1998.12.3 */
    }

    /* 自杀功能,Luzi 1998.10.10 */
    if (urec->userlevel & PERM_SUICIDE)
        return (LIFE_DAY_SUICIDE * 24 * 60 - value)/(60*24);
    /**********************/
    if(urec->numlogins <= 3)
        return (LIFE_DAY_SUICIDE * 24 * 60 - value)/(60*24);
    if( !(urec->userlevel & PERM_LOGINOK) )
        return (LIFE_DAY_NEW * 24 * 60 - value)/(60*24);
    /* if (urec->userlevel & PERM_LONGID)
         return (667 * 24 * 60 - value)/(60*24); */
     registeryear = (time(0) -urec->firstlogin)/31536000;
    if (registeryear<2) basiclife = LIFE_DAY_USER ;
    else  if (registeryear >=5) basiclife = LIFE_DAY_LONG;
       else basiclife = LIFE_DAY_YEAR;
    return (basiclife * 24 * 60 - value)/(60*24);
}


char *setbdir(int digestmode,char *buf, char *boardname)
  /* 根据阅读模式 取某版 目录路径 */
{
    char dir[STRLEN];

    switch(digestmode)
    {
    case NA:
        strcpy(dir,DOT_DIR);
        break;
    case YEA:
        strcpy(dir,DIGEST_DIR);
        break;
    case 2:
        strcpy(dir,THREAD_DIR);
        break;
    case 4:
	strcpy(dir,".DELETED");
	break;
    case 5:
	strcpy(dir,".JUNK");
	break;
    }
    sprintf( buf, "boards/%s/%s", boardname, dir);
    return buf;
}



char *
sethomefile( char    *buf,char  *userid,char *filename)  /*取某用户文件 路径*/
{
    if (isalpha(userid[0]))  /* 加入错误判断,提高容错性, alex 1997.1.6*/
        sprintf( buf, "home/%c/%s/%s", toupper(userid[0]), userid, filename );
    else
        sprintf( buf, "home/wrong/%s/%s", userid, filename);
    return buf;
}

char *
sethomepath( char    *buf, char *userid)  /* 取 某用户 的home */
{
    if (isalpha(userid[0]))  /* 加入错误判断,提高容错性, alex 1997.1.6*/
        sprintf( buf, "home/%c/%s", toupper(userid[0]), userid );
    else
        sprintf( buf, "home/wrong/%s", userid);
    return buf;
}

char *
setmailfile( char    *buf, char *userid, char *filename)    /* 取某用户mail文件 路径*/
{
    if (isalpha(userid[0]))  /* 加入错误判断,提高容错性, alex 1997.1.6*/
        sprintf( buf, "mail/%c/%s/%s", toupper(userid[0]), userid, filename );
    else
        sprintf( buf, "mail/wrong/%s/%s", userid, filename);
    return buf;
}

char * setmailpath( char    *buf, char *userid)  /* 取 某用户 的mail */
{
    if (isalpha(userid[0]))  /* 加入错误判断,提高容错性, alex 1997.1.6*/
        sprintf( buf, "mail/%c/%s", toupper(userid[0]), userid );
    else
        sprintf( buf, "mail/wrong/%s", userid);
    return buf;
}

char * setbpath( char *buf, char *boardname)   /* 取某版 路径 */
{
    strcpy( buf, "boards/" );
    strcat( buf, boardname );
    return buf;
}

char *setbfile( char *buf, char *boardname, char *filename)  /* 取某版下文件 */
{
    sprintf( buf, "boards/%s/%s", boardname, filename );
    return buf;
}

void RemoveMsgCountFile(char *userID)
{
  char fname[STRLEN];
  sethomefile(fname,userID,"msgcount");
  unlink(fname);
 }


int id_invalid(char* userid)
{
    char *s;
    if (!isalpha(userid[0]))
        return 1;
    for(s=userid;*s != '\0'; s++) {
        if(*s<1 || !isalnum(*s)) {
            return 1;
        }
    }
    return 0;
}


int seek_in_file(char filename[STRLEN],char seekstr[STRLEN])
{
    FILE *fp;
    char buf[STRLEN];
    char *namep;

    if ((fp = fopen(filename, "r")) == NULL)
        return 0;
    while (fgets(buf, STRLEN, fp) != NULL)
	{
        namep = (char *)strtok( buf, ": \n\r\t" );
        if (namep != NULL && strcasecmp(namep, seekstr) == 0 ) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

static struct public_data *publicshm;
struct public_data* get_publicshm()
{
   int iscreate;
    if (publicshm==NULL) {
        publicshm = attach_shm1( NULL, PUBLIC_SHMKEY, sizeof( *publicshm) ,&iscreate,1,NULL); /* attach public share memory readonly*/
		if (iscreate) {
			bbslog("4bbstime","time daemon not start");
			exit(1);
		}
    }

    return publicshm;
}

int getwwwguestcount()
{
	return publicshm->www_guest_count;
}

void bbssettime(time_t now)
{
    int iscreate;
    if (publicshm==NULL) {
        publicshm = (struct public_data*)attach_shm1( NULL, PUBLIC_SHMKEY, sizeof( *publicshm) ,&iscreate,0,NULL); /* attach public share memory*/
        if (iscreate) {
        	/* 初始化public共享内存区*/
        	/* 开始的sysconf.img版本号为0*/
        	unlink("sysconf.img.0");
        	publicshm->sysconfimg_version=0;
        	
        }
    }
    publicshm->nowtime=now;
    return;
}


int setpublicshmreadonly(int readonly)
{
    int iscreate;
    shmdt(publicshm);
    if (readonly)
        publicshm = (struct public_data*)attach_shm1( NULL, PUBLIC_SHMKEY, sizeof( *publicshm ) ,&iscreate , 1, publicshm); 
    else
        publicshm = (struct public_data*)attach_shm1( NULL, PUBLIC_SHMKEY, sizeof( *publicshm ) ,&iscreate , 0, publicshm); 
    return (publicshm==NULL);
}

time_t bbstime(time_t* t)
{
    int iscreate;
    if (publicshm==NULL) {
        publicshm = attach_shm1( NULL, PUBLIC_SHMKEY, sizeof( *publicshm) ,&iscreate,1,NULL); /* attach public share memory readonly*/
		if (iscreate) {
			bbslog("4bbstime","time daemon not start");
			exit(1);
		}
    }
    if (t) *t=publicshm->nowtime;
    return publicshm->nowtime;
}

int bad_user_id( char    *userid)
{
    FILE        *fp;
    char        buf[STRLEN];
    char        *ptr, ch;
    int         i;

    i = 0;
    ptr = userid;
    if (!strcasecmp(userid,"deliver")||!strcasecmp(userid,"new"))
    	return 1;
    while( (ch = *ptr++) != '\0' ) {
        i++;
        if( !isalnum( ch ) && ch != '_' )
            return 1;
    }
    if (i<2) return 1;
    if( (fp = fopen( ".badname", "r" )) != NULL ) {
        while( fgets( buf, STRLEN, fp ) != NULL ) {
            ptr = strtok( buf, " \n\t\r" );
            if( ptr != NULL && *ptr != '#')
            {
                if( strcasecmp( ptr, userid ) == 0 ) {
                    if(ptr[13]>47 && ptr[13]<58)/*Haohmaru.99.12.24*/
                    {
                        char timebuf[50];
                        time_t	t,now;
                        strncpy(timebuf,ptr+13,49);
			timebuf[49] = 0;
                        ptr = timebuf;
                        while (isdigit(*ptr)) ptr++;
			*ptr=0;
                        t = atol(timebuf);
                        now = time(0);
                        if(now - t > 24*30*3600)
                        {
                            fclose( fp );
                            return 0;
                        }
                    }
                    fclose(fp);
                    return 1;
                }
            }
	    bzero(buf,STRLEN);
        }
        fclose(fp);
    }
    return 0;
}

int valid_ident( char *ident)
{
    int         i;

    for( i = 0; invalid[i] != NULL; i++ )
        if( strstr( ident, invalid[i] ) != NULL )
            return 0;
    return 1;
}

struct _tag_t_search {
	struct user_info* result;
	int pid;
};

int _t_search(struct user_info* uentp,struct _tag_t_search* data,int pos)
{
	if (data->pid==0) {
		data->result=uentp;
		return QUIT;
	}
	data->result=uentp;
	if (uentp->pid==data->pid)
		return QUIT;
	UNUSED_ARG(pos);
	return 0;
}

struct user_info * t_search(char *sid,int  pid)
{
    struct _tag_t_search data;

    data.pid=pid;
    data.result=NULL;

    apply_utmp((APPLY_UTMP_FUNC)_t_search,20,sid,&data);
    
    return data.result;
}

int getuinfopid()
{
#ifdef BBSMAIN
   return uinfo.pid;
#else
   return 1;
#endif
}

int cmpinames(const char *userid,const  char *uv)       /* added by Luzi 1997.11.28 */
{
    return !strcasecmp(userid, uv);
}

int cmpfnames(char    *userid, struct friends *uv)
{       
	    return !strcasecmp(userid, uv->id);
}       
  
int
canIsend2(userid) /* Leeward 98.04.10 */
char *userid;
{
    char buf[IDLEN+1];
    char path[256];

    if (HAS_PERM(currentuser,PERM_SYSOP)) return YEA;

    sethomefile( path, userid , "/ignores");
    if (search_record(path, buf, IDLEN+1, (RECORD_FUNC_ARG)cmpinames, currentuser->userid))
        return NA;
    sethomefile( path, userid , "/bads");
    if (search_record(path, buf, IDLEN+1, (RECORD_FUNC_ARG)cmpinames, currentuser->userid))
        return NA;
    else
        return YEA;
}

sigjmp_buf bus_jump;
void sigbus(int signo)
{
  siglongjmp(bus_jump,1);
};

int safe_mmapfile(char* filename,int openflag,int prot,int flag,void** ret_ptr,int* size,int* ret_fd)
{
	int fd;
	struct stat st;
	fd = open(filename, openflag,0600);
	if (fd < 0)
		return 0;
	if (fstat(fd, &st) < 0) {
		close(fd);
		return 0;
	}
	if (!S_ISREG(st.st_mode)) {
		close(fd);
		return 0;
	}
	if (st.st_size <= 0) {
		close(fd);
		return 0;
	}
	*ret_ptr = mmap(NULL, st.st_size, prot, flag, fd, 0);
	if (!ret_fd)
		close(fd);
	else {
		*ret_fd=fd;
		flock(fd,LOCK_EX);
	}
	if (*ret_ptr == NULL)
		return 0;
    if (!sigsetjmp(bus_jump,1)) {
        signal(SIGBUS,sigbus);
		/*
		signal(SIGSEGV,sigbus);
		*/
		*size=st.st_size;
		return 1;
    }
    return 2;
}

void end_mmapfile(void* ptr,int size,int fd)
{
 	munmap(ptr, size);
    signal(SIGBUS,SIG_IGN);
	/*
    signal(SIGSEGV,SIG_IGN);
	*/
    if (fd!=-1) close(fd);
}

void
encodestr( register char *str )
{
    register char       ch, *buf;
    int                 n;

    buf = str;
    while( (ch = *str++) != '\0' ) {
        if( *str == ch && str[1] == ch && str[2] == ch ) {
            n = 4;
            str += 3;
            while( *str == ch && n < 100 ) {
                str++;
                n++;
            }
            *buf++ = '\01';
            *buf++ = ch;
            *buf++ = n;
        } else
            *buf++ = ch;
    }
    *buf = '\0';
}

int addtofile(filename,str)
char filename[STRLEN],str[STRLEN];
{
    FILE *fp;
    int rc;

    if ((fp = fopen(filename, "a")) == NULL)
        return -1;
    flock(fileno(fp), LOCK_EX);
    rc = fprintf( fp, "%s\n",str);
    flock(fileno(fp), LOCK_UN);
    fclose(fp);
    return(rc == EOF ? -1 : 1);
}

time_t get_exit_time(id,exittime) /* 获取离线时间，id:用户ID,
                                   exittime:保存返回的时间，结束符为\n
                                            建议定义为 char exittime[40]
                                   Luzi 1998/10/23 */
/* Leeward 98.10.26 add return value: time_t */
char *id;
char *exittime;
{
    char path[80];
    FILE *fp;
    time_t now = 1; /* if fopen failed return 1 -- Leeward */
    sethomefile( path, id , "exit");
    fp=fopen(path, "rb");
    if (fp!=NULL)
    {
        fread(&now,sizeof(time_t),1,fp);
        fclose(fp);
        strcpy(exittime, ctime(&now));
    }
    else exittime[0]='\n';

    return now;
}

void getuinfo(FILE *fn,struct userec *ptr_urec)
{
    fprintf(fn,"\n\n您的代号     : %s\n", ptr_urec->userid);
    fprintf(fn,"您的昵称     : %s\n", ptr_urec->username);
    fprintf(fn,"真实姓名     : %s\n", ptr_urec->realname);
    fprintf(fn,"居住住址     : %s\n", ptr_urec->address);
    fprintf(fn,"电子邮件信箱 : %s\n", ptr_urec->email);
    fprintf(fn,"真实 E-mail  : %s\n", ptr_urec->realemail);
    fprintf(fn,"Ident 资料   : %s\n", ptr_urec->ident);
    fprintf(fn,"注册日期     : %s", ctime( &ptr_urec->firstlogin));
    fprintf(fn,"最近光临日期 : %s", ctime( &ptr_urec->lastlogin));
    fprintf(fn,"最近光临机器 : %s\n", ptr_urec->lasthost );
    fprintf(fn,"上站次数     : %d 次\n", ptr_urec->numlogins);
    fprintf(fn,"文章数目     : %d (Board)\n",
            ptr_urec->numposts);
}


int del_from_file(char filename[STRLEN],char str[STRLEN])
{
    FILE *fp, *nfp;
    int deleted = NA;
    char fnnew[256/*STRLEN*/];
    char buf[256/*STRLEN*/];

    if ((fp = fopen(filename, "r")) == NULL) return -1;
    sprintf( fnnew, "%s.%d", filename, getuid());
    if ((nfp = fopen(fnnew, "w")) == NULL) return -1;
    while(fgets(buf, 256/*STRLEN*/, fp) != NULL) {
        if( strncasecmp(buf, str, strlen(str)) == 0 && buf[strlen(str)] <= 32)
            deleted = YEA;
        else if( *buf > ' ' )
            fputs(buf, nfp);
    }
    fclose(fp);
    fclose(nfp);
    if (!deleted) return -1;
    return(f_mv(fnnew, filename));
}

int
simplepasswd( str )
char *str;
{
    char        ch;

    while( (ch = *str++) != '\0' ) {
        if( ! (ch >= 'a' && ch <= 'z') )
            return 0;
    }
    return 1;
}

void
logattempt( uid, frm )
char *uid, *frm;
{
    char        fname[ STRLEN ];
    int         fd, len;
	char buf[256];

    snprintf( buf, sizeof(buf), "%-12.12s  %-30s %s\n",
                uid, Ctime( bbstime(0)), frm );
    len = strlen( buf );
    if( (fd = open( BADLOGINFILE, O_WRONLY|O_CREAT|O_APPEND, 0644 )) >= 0 ) {
        write( fd, buf, len );
        close( fd );
    }
    sethomefile( fname, uid, BADLOGINFILE );
    if( (fd = open( fname, O_WRONLY|O_CREAT|O_APPEND, 0644 )) >= 0 ) {
        write( fd, buf, len );
        close( fd );
    }
}

int
check_ban_IP(char *IP, char *buf)
{ /* Leeward 98.07.31
  RETURN:
 - 1: No any banned IP is defined now
 0: The checked IP is not banned
  other value over 0: The checked IP is banned, the reason is put in buf
  */
  FILE *Ban = fopen(".badIP", "r");
  char IPBan[64];
  int  IPX = - 1;
  char *ptr;

  if (!Ban)
    return IPX;
  else
    IPX ++;

  while (fgets(IPBan, 64, Ban))
  {
    if ((ptr = strchr(IPBan, '\n'))!=NULL)
      *ptr = 0;
    if ((ptr = strchr(IPBan, ' '))!=NULL)
    {
      *ptr ++ = 0;
      strcpy(buf, ptr);
    }
    IPX = strlen(IPBan);
    if (*IPBan=='+')
        if (!strncmp(IP, IPBan+1, IPX-1))
          break;
    IPX = 0;
  }

  fclose(Ban);
  return IPX;
}

