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

#include "bbs.h"
#include "screen.h" /* Leeward 98.06.05 */

time_t calltime=0;
void R_monitor();

/*struct FILESHM
{
        char line[FILE_MAXLINE][FILE_BUFSIZE];
        int  fileline;
        int  max;
        time_t  update;
};
struct  FILESHM   *goodbyeshm;
struct  FILESHM   *issueshm;
*/
struct ACSHM
{
    char line[ACBOARD_MAXLINE][ACBOARD_BUFSIZE];
    int  movielines;
    time_t  update;
};

struct  ACSHM   *movieshm;

int     nnline = 0, xxxline = 0;
int     more_size, more_num;

int
NNread_init()
{
    FILE *fffd ;
    char *ptr;
    char buf[ACBOARD_BUFSIZE];
    struct stat st ;
    time_t         ftime,now;
    int iscreate;

    now=time(0);
    if( stat( "etc/movie",&st ) < 0 ) {
        return 0;
    }
    ftime = st.st_mtime;
    if(  movieshm== NULL ) {
        movieshm = (void *)attach_shm( "ACBOARD_SHMKEY", 4123, sizeof( *movieshm ),&iscreate );
    }
    if(abs(now-movieshm->update)<12*60*60&&ftime<movieshm->update)
    {
        return 1;
    }
    /*---	modified by period	2000-10-20	---*
            if ((fffd = fopen( "etc/movie" , "r" )) == NULL) {
                    return 0;
            }         
     ---*/
    nnline = 0;
    xxxline = 0;
    if(!DEFINE(DEF_ACBOARD))
    {
        nnline = 1;
        xxxline = 1;
        return 1;
    }
    /*---	原有程序顺序有误, !DEFINE --> return后没close	---*/
    if ((fffd = fopen( "etc/movie" , "r" )) == NULL)
        return 0;
    /*---	---*/
    while ((xxxline < ACBOARD_MAXLINE) &&
            (fgets( buf,ACBOARD_BUFSIZE, fffd )!=NULL)) {
        ptr=movieshm->line[xxxline];
        memcpy( ptr, buf, sizeof(buf) );
        xxxline++;
    }
    sprintf(buf,"%79.79s\n"," ");
    movieshm->movielines=xxxline;
    while(xxxline%MAXnettyLN!=0)
    {
        ptr=movieshm->line[xxxline];
        memcpy( ptr, buf, sizeof(buf) );
        xxxline++;
    }
    movieshm->movielines=xxxline;
    movieshm->update=time(0);
    sprintf(buf,"%d 行 活动看板 更新",xxxline);
    report(buf);
    fclose(fffd);
    return 1;
}

void
check_calltime()
{
    int line;

    if(time(0)>=calltime&&calltime!=0)
    {
        /*         if (uinfo.mode != MMENU)
                 {
                      bell();
                      move(0,0);
                      clrtoeol();
                      prints("请到主选单看备忘录......");
                      return;
                  }
		set_alarm(0,NULL,NULL);
                showusernote();
                pressreturn();
                R_monitor();
        */
        if(uinfo.mode==TALK)
            line=t_lines/2-1;
        else
            line=0;
        saveline(line, 0, NULL); /* restore line */
        bell();bell();bell();
        move(line,0);
        clrtoeol();
        prints("[44m[32mBBS 系统通告: [37m%-65s[m","系统闹钟 铃～～～～～～");
        igetkey();
        move(line,0);
        clrtoeol();
        saveline(line, 1, NULL);
        calltime=0;
    }
}

void
setcalltime()
{
    char ans[6];
    int ttt;

    move(1,0);
    clrtoeol();
    getdata(1,0,"几分钟後要系统提醒你: ",ans,3,DOECHO,NULL,YEA);
    if(!isdigit(ans[0]))
        return;
    ttt=atoi(ans);
    if(ttt<=0)
        return;
    calltime=time(0)+ttt*60;

}

int
readln(int fd,char* buf,char* more_buf)
{
    int len, bytes, in_esc, ch;

    len = bytes = in_esc = 0;
    while(1) {
        if( more_num >= more_size ) {
            more_size = read( fd, more_buf, MORE_BUFSIZE );
            if( more_size == 0 ) {
                break;
            }
            more_num = 0;
        }
        ch = more_buf[ more_num++ ];
        bytes++;
        if( ch == '\n' || bytes > 255 ) {
            break;
        } else if( ch == '\t' ) {
            do {
                len++, *buf++ = ' ';
            } while( (len % 8) != 0 );
        } else if( ch == '' ) {
            if( showansi )  *buf++ = ch;
            in_esc = 1;
        } else if( in_esc ) {
            if( showansi )  *buf++ = ch;
            if( strchr( "[0123456789;,", ch ) == NULL ) {
                in_esc = 0;
            }
        } else if( isprint2( ch ) ) {
            /*if(len>79)
                break;*/
            len++, *buf++ = ch;
        }
    }
    *buf++ = ch;
    *buf = '\0';
    return bytes;
}

int
morekey()
{
    while( 1 ) {
        switch( egetch() ) {
        case Ctrl('Z'):
                        return 'M'; /* Leeward 98.07.30 support msgX */
        case '!': return '!'; /*Haohmaru 98.09.24*/
case 'q':  case KEY_LEFT:  case EOF:
            return KEY_LEFT;
    case ' ':  case KEY_RIGHT:
    case KEY_PGDN: case Ctrl('F'):
                        return KEY_RIGHT;
    case KEY_PGUP : case Ctrl('B'):
                        return KEY_PGUP;
case '\r': case KEY_DOWN: case 'j':
            return KEY_DOWN;
    case 'k' : case KEY_UP:
            return KEY_UP;
            /*************** 新增加阅读时的热键 Luzi 1997.11.1 ****************/
    case 'h':  case '?':
            return 'H';
    case 'o':  case 'O':
            return 'O';
    case 'l':  case 'L':
            return 'L';
    case 'w':  case 'W':
            return 'W';
        case 'H':
            return 'M';
        case 'X': /* Leeward 98.06.05 */
            return 'X';
        case 'u': /*Haohmaru 99.11.29 */
            return 'u';
        default : ;
        }
    }
}

int seek_nth_line(int fd, int no,char* more_buf)
{
    int  n_read, line_count, viewed;
    char *p, *end;

    lseek(fd, 0, SEEK_SET );
    line_count = viewed = 0;
    if ( no > 0 ) while ( 1 ) {
            n_read = read( fd, more_buf, MORE_BUFSIZE );
            p = more_buf; end = p + n_read;
            for ( ; p < end && line_count < no; p++)
                if (*p == '\n') line_count++;
            if (line_count >= no) {
                viewed += ( p - more_buf );
                lseek(fd, viewed, SEEK_SET);
                break;
            } else viewed += n_read;
        }

    more_num = MORE_BUFSIZE + 1;  /* invalidate the readln()'s buffer */
    return viewed;
}

/*Add by SmallPig*/
int
countln(fname)
char *fname;
{
    FILE  *fp;
    char  tmp[256];
    int   count=0;

    if((fp=fopen(fname,"r"))==NULL)
        return 0;

    while(fgets(tmp,sizeof(tmp),fp)!=NULL)
        count++;
    fclose(fp);
    return count;
}

int
more(filename,promptend)
char    *filename ;
int     promptend;
{
    showansi = 0;
    return rawmore( filename, promptend);
    showansi = 1;
}

/* below added by netty  *//*Rewrite by SmallPig*/
void
netty_more()
{
    char buf[256];
    int         ne_row = 1;
    int x,y;
    time_t thetime = time(0);

    if(!DEFINE(DEF_ACBOARD))
    {
        update_endline();
        return ;
    }

    nnline=((thetime/10)%(movieshm->movielines/MAXnettyLN))*MAXnettyLN;

    getyx(&y,&x);
    update_endline();
    move(3,0);
    while ((nnline < movieshm->movielines)/*&&DEFINE(DEF_ACBOARD)*/) {
        move(2+ne_row,0);
        clrtoeol();

        strcpy(buf,movieshm->line[ nnline ]);
        showstuff(buf);
        nnline = nnline + 1;
        ne_row = ne_row + 1;
        if (nnline == movieshm->movielines) { nnline = 0;  break; }
        if (ne_row > MAXnettyLN) {
            break;
        }
    }
    move (y,x);
}

printacbar()
{
    int x,y;
    getyx(&y,&x);

    move(2,0);
    prints("[35m┌——————————————┤[37m活  动  看  版[35m├——————————————┐ [m\n");
    move(3+MAXnettyLN,0);
    prints("[35m└——————————————┤[36m"FOOTER_MOVIE"[35m├——————————————┘ [m\n");
    move (y,x);
    refresh();
}

extern int idle_count;
void R_monitor(void* data)
{
    if(!DEFINE(DEF_ACBOARD))
        return;

    if (uinfo.mode != MMENU)
        return;
    netty_more();
    printacbar();
    idle_count++;
    set_alarm(10*idle_count,R_monitor,NULL);
    UNUSED_ARG(data); 
}


/*rawmore2() ansimore2() Add by SmaLLPig*/
int
rawmore(filename,promptend,row,numlines)
char    *filename;
int     promptend;
int     row;
int     numlines;
{
    extern int  t_lines ;
    struct stat st ;
    int         fd, tsize;
    char        buf[ 256 ] ;
    int         i, ch, viewed, pos ,isin=NA,titleshow=NA;
    int         numbytes ;
    int         curr_row = row;
    int         linesread = 0;
	char        more_buf[ MORE_BUFSIZE ];

    if( (fd = open(filename,O_RDONLY)) == -1 ) {
        return -1;
    }
    if( fstat( fd, &st ) ) {
        close(fd);  /*---   period 2000-10-20 file should be closed here   ---*/
        return -1;
    }
    if(!S_ISREG(st.st_mode))
	return-1;
    tsize = st.st_size ;
    more_size = more_num = 0;

    clrtobot();
    i = pos = viewed = 0 ;
    numbytes = readln(fd,buf,more_buf) ;  curr_row++; linesread++;
    /*if (numbytes)
      {
        char *lpTmp;
        lpTmp=strstr(buf,"读者数");
        if (lpTmp)
          { FILE *fp;
            memset(lpTmp,32,15);
            lpTmp[15]=NULL;
            fp=fopen(filename,"rb+");
            fwrite(buf,strlen(buf),1,fp);
            fclose(fp);
            lpTmp[15]=32;
           }
        }
     */       
    while( numbytes )
    {
        if(linesread <= numlines || numlines == 0)
        {
            viewed += numbytes ;
            if(!titleshow&&
                    (!strncmp(buf,"□ 引用",7))
                    ||(!strncmp(buf,"==>",3))
                    ||(!strncmp(buf,"【 在",5))
                    ||(!strncmp(buf,"※ 引述",7)))
            {
                prints("[33m%s[m",buf);
                titleshow=YEA;
            }else if(buf[0]!=':'&&buf[0]!='>')
            {
                if(isin==YEA)
                {
                    isin=NA;
                    prints("[m");
                }
                if(check_stuffmode())
                    showstuff(buf);
                else
                    prints("%s",buf);
            }
            else
            {
                prints("[36m");
                if(check_stuffmode())
                    showstuff(buf);
                else
                    prints("%s",buf);
                isin=YEA;
            }
            i++ ;
            pos++ ;
            if(pos == t_lines)
            {
                scroll() ;
                pos-- ;
            }
            numbytes = readln(fd,buf,more_buf) ; curr_row++; linesread++;
            if( numbytes == 0 )
                break ;
            if( i == t_lines -1 )
            {
                if( showansi )
                {
                    move( t_lines-1, 0 ) ;
                    prints( "[37;40;0m[m" );
                    refresh();
                }
                move( t_lines-1, 0 ) ;
                clrtoeol();
                prints("[44m[32m下面还有喔 (%d%%)[33m   │ 结束 ← <q> │ ↑/↓/PgUp/PgDn 移动 │ ? 辅助说明 │     [m", (viewed*100)/tsize);
                ch = morekey();
                move( t_lines-1, 0 );
                clrtoeol();
                refresh();
                if( ch == KEY_LEFT )
                {
                    close( fd );
                    return ch;
                } else if( ch == KEY_RIGHT )
                {
                    i = 1;
                } else if( ch == KEY_DOWN )
                {
                    i = t_lines-2 ;
                } else if(ch == KEY_PGUP || ch == KEY_UP)
                {
                    clear(); i = pos = 0;
                    curr_row -= (ch == KEY_PGUP) ? (2 * t_lines - 2) : (t_lines + 1);
                    if (curr_row < 0) { close( fd ); return ch; }
                    viewed = seek_nth_line(fd, curr_row,more_buf);
                    numbytes = readln(fd,buf,more_buf) ;  curr_row++;
                } else if(ch == 'H')
                {
                    show_help( "help/morehelp" );
                    i = pos = 0;
                    curr_row -= (t_lines);
                    if (curr_row < 0) curr_row = 0;
                    viewed = seek_nth_line(fd, curr_row,more_buf);
                    numbytes = readln(fd,buf,more_buf) ;  curr_row++;
                }
                /* Luzi  新增阅读热键 1997.11.1 */
                else if (ch=='O')
                    { if (HAS_PERM(PERM_BASIC))
                    {
                        t_friends();
                        i = pos = 0;
                        curr_row -= (t_lines);
                        if (curr_row < 0) curr_row = 0;
                        viewed = seek_nth_line(fd, curr_row,more_buf);
                        numbytes = readln(fd,buf,more_buf) ;  curr_row++;
                    }
                }
                else if (ch=='!')/*Haohmaru 98.09.24 快速离站*/
                {
                    Goodbye();
                    i = pos = 0;
                    curr_row -= (t_lines);
                    if (curr_row < 0) curr_row = 0;
                    viewed = seek_nth_line(fd, curr_row,more_buf);
                    numbytes = readln(fd,buf,more_buf) ;  curr_row++;
                }
                else if (ch=='L')
                {
                    if(uinfo.mode!=LOOKMSGS)
                    {
                        show_allmsgs();
                        i = pos = 0;
                        curr_row -= (t_lines);
                        if (curr_row < 0) curr_row = 0;
                        viewed = seek_nth_line(fd, curr_row,more_buf);
                        numbytes = readln(fd,buf,more_buf) ;  curr_row++;
                    }
                }
                else if (ch=='M')
                {
                    r_lastmsg();
                    clear();
                    i = pos = 0;
                    curr_row -= (t_lines);
                    if (curr_row < 0) curr_row = 0;
                    viewed = seek_nth_line(fd, curr_row,more_buf);
                    numbytes = readln(fd,buf,more_buf) ;  curr_row++;
                }
                else if (ch=='W')
                    { if (HAS_PERM(PERM_PAGE))
                    {
                        s_msg();
                        i = pos = 0;
                        curr_row -= (t_lines);
                        if (curr_row < 0) curr_row = 0;
                        viewed = seek_nth_line(fd, curr_row,more_buf);
                        numbytes = readln(fd,buf,more_buf) ;  curr_row++;
                    }
                }
                else if (ch == 'u')
                {
                    int oldmode = uinfo.mode;
                    clear();
                    modify_user_mode(QUERY);
                    t_query();
                    clear();
                    modify_user_mode(oldmode);
                    i = pos = 0;
                    curr_row -= (t_lines);
                    if (curr_row < 0) curr_row = 0;
                    viewed = seek_nth_line(fd, curr_row,more_buf);
                    numbytes = readln(fd,buf,more_buf) ;  curr_row++;
                }
                else if ('X' == ch) /* Leeward 98.06.05 */
                {
                    extern struct screenline *big_picture;
                    char buffer[256];
                    char *pID;
                    char err[] = "Leeward未预料到的错误(XCL)";
                    char userid[IDLEN + 1];
                    int  count;

                    sprintf(buffer, "tmp/XCL.%s%d", currentuser->userid, getpid());
                    if (HAS_PERM(PERM_ADMIN) && HAS_PERM(PERM_SYSOP)
                            &&  !strcmp(buffer, filename))
                    {
                        for (count = 0; count < t_lines; count ++)
                            if (pID = strstr(big_picture[count].data, "用户代号(昵称) : ")) break;
                        pID = pID ? pID + 17 : err;
                        sprintf(buffer, "给 %s", pID);
                        pID = strchr(buffer, '(');
                        if (pID) *pID = 0;
                        strncpy(userid, buffer + 3, IDLEN);
                        strcat(buffer, " 发信，要求其补齐个人注册资料");
                        if (askyn(buffer, 0))
                        {
                            struct fileheader newmessage ;
                            char filepath[256];
                            FILE *fp;
                            time_t now = time(0);
                            int id; 
			    unsigned int newlevel;

                            memset(&newmessage, 0,sizeof(newmessage)) ;
                            strcpy(newmessage.owner,"站务管理系统");
                            strcpy(newmessage.title,"您的个人资料不够详实，请到个人工具箱内补充设定") ;
                            sprintf(buffer, "M.%d.XCL.%s%d", now, currentuser->userid, getpid());
                            setmailfile(filepath, userid, buffer);
                            strcpy(newmessage.filename,buffer) ;
                            fp = fopen(filepath, "w");
                            if (fp)
                            {
                                fprintf(fp, "寄信人: BBS "NAME_BBS_CHINESE"站"NAME_MATTER"管理系统\n");
                                fprintf(fp, "标  题: 您的个人资料不够详实，请到个人工具箱内补充设定\n");
                                fprintf(fp, "发信站: BBS "NAME_BBS_CHINESE"站 (%24.24s)\n",ctime(&now)) ;
                                fprintf(fp,"来  源: %s \n\n",currentuser->lasthost) ;
                                fprintf(fp, "[1m[33m您的个人资料不够详实，请马上到个人工具箱内补充设定。[0m[0m\n\n");
                                fprintf(fp, "设定时请注意：\n\n"
                                        "        1) 请填写真实姓名.           (可用拼音)\n"
                                        "        2) 请详填学校科系与年级.     (注明年级或工作单位，没有单位的写无业)\n"
                                        "        3) 请填写完整的住址资料.     (完整详细，能够通信)\n"
                                        "        4) 请详填连络电话.           (没有电话请注明“无”)\n"
                                        "        5) 请填写有效的电子邮件信箱. (能够通信，没有请注明“无”)\n"
                                        "        6) 不允许使用不雅或者是不恰当的昵称.\n"
                                        "        7) 请尽量使用中文填写.\n\n");
                                fprintf(fp, "[1m[31m目前，站务管理系统已暂时取消您在本站的站务管理权限，\n等您将个人资料补充设定合格后，请写信给主管您职务的站长申请恢复权限。[0m[0m\n");

                                fclose(fp);
                            }
                            else
                                a_prompt(-1, "警告：无法生成信件正文，请按任何键以继续 <<", buffer);
                            setmailfile(buffer, userid, DOT_DIR);
                            if(append_record(buffer,&newmessage,sizeof(newmessage)) == -1) a_prompt(-1, "错误：无法投递信件，请按任何键以继续 <<", buffer);
                            else {
                                struct userec* lookupuser;
                                sprintf(buffer, "mailed %s ", userid);
                                report(buffer);
                                id = getuser(userid,&lookupuser) ;
                                newlevel = lookupuser->userlevel;
                                newlevel &= ~PERM_BOARDS;
                                newlevel &= ~PERM_SYSOP;
                                newlevel &= ~PERM_OBOARDS;
                                newlevel &= ~PERM_ADMIN;
                                if (newlevel != lookupuser->userlevel)
                                {
                                    sprintf(buffer,"修改 %s 的权限XPERM%d %d",
                                            lookupuser->userid, lookupuser->userlevel, newlevel);
                                    securityreport(buffer,lookupuser);
                                    lookupuser->userlevel = newlevel;
                                    sprintf(buffer, "changed permissions for %s", lookupuser->userid);
                                    report(buffer);
                                    a_prompt(-1, "已发送信件说明要求，并取消相关权限，请按任何键以继续 <<", buffer);
                                }
                                else a_prompt(-1, "相关权限早已取消，仅发送信件说明要求，请按任何键以继续 <<", buffer);
                            }
                        }
                    }

                    clear();
                    i = pos = 0;
                    curr_row -= (t_lines);
                    if (curr_row < 0) curr_row = 0;
                    viewed = seek_nth_line(fd, curr_row,more_buf);
                    numbytes = readln(fd,buf,more_buf) ;  curr_row++;
                }
            }
        }
        else break;/*More Than Want*/
    }
    close( fd ) ;
    if( promptend ) {
        pressanykey();
    }
    return 0 ;
}

int
ansimore(filename,promptend)
char    *filename ;
int     promptend;
{
    int ch;

    clear();
    ch = rawmore( filename, promptend ,0 , 0);
    move( t_lines-1, 0 );
    prints( "[37;40;0m[m" );
    refresh();
    return ch;
}

int
ansimore2(filename,promptend,row,numlines)
char    *filename ;
int     promptend;
int     row;
int     numlines;
{
    int ch;
    ch = rawmore( filename, promptend, row, numlines );
    refresh();
    return ch;
}
