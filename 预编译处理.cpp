#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define LINESIZE 512  //���ļ�ʱÿ�������ȡ���ַ���
#define VTSIZE 1024
#define CTSIZE  1024

#define IDENTIFY 1
#define KEYWORD 2
#define CONSTANT 3
#define STRING 4
#define OPERATOR 5
#define BORDER 6

typedef struct ConTable_non
{
    int id_non;
    char name_non[100];
    char replace_non[100];
    struct ConTable_non *next_non;
} ConTable_non;  //�޲κ��㶨��

typedef struct ReplaceNode
{
    int id_flag;
    char string_section[10];
    struct ReplaceNode *next_replace;
}ReplaceNode;  //����ַ����Ľڵ�

typedef struct ConTable
{
    int id;
    char name[100];
    int argu_count;
    char argu_list[20];  //Ĭ�϶�����κ�ʱ��ÿ����������һ���ַ�
    ReplaceNode *replace;
    struct ConTable *next;
} ConTable;  //���κ��㶨��

typedef struct Token
{
    int line;
    int number;
    char name[100];
    int x;  //�к�
    int y;  //�к�
    int times;
	struct Token *next;
} Token;

typedef struct IdenTable
{
    int id;
    char name[20];
    int times;
} IdenTable;

typedef struct ConTable_token
{
    int id;
    char name[100];
    int times;
} ConTable_token;

typedef struct HashNode
{
	char name[10];
	int times;
	struct HashNode *next;
} HashNode;

char buf[LINESIZE];
int start;  //���鵱ǰλ�ñ��
FILE *in,*out;
char *define_addr;
int flag_def=0;
int flag_ifdef=1;  //ͨ��flag_ifdef���ж��Ƿ���뵱ǰ���
char judge[6][20];

/**�궨��������ָ��**/
ConTable_non *head_non,*tail_non;
ConTable *head,*tail;
int keywordtimes[34]={0};
int instructtimes[40]={0};
int bordertimes[4]={0};
int changetimes[12]={0};

char instructList[][4] = { "+","-","*","/","%","{","}",")","(",">","<","=","[","]","?","!","&","|",
                       "++","--","==",":=",">=","<=","!=","&&","||","+=","-=","*=","/=","^",">>","<<","~","#",".","\""
                     };//by order unnecessary notation
char borderList[][3]={";",",",":"};
char keywordList[][10] = {"include","auto","break","case","char","const","continue","default","do","double",
                       "else","enum","extern","float","for","goto","if","int","long","register",
                       "return","short","signed","sizeof","static","struct","switch","typedef",
                       "union","unsigned","volatile","while",""
                      };//by order
char changeList[12] = {'a', 'b', 'f','n','r','t','v','\\','?','"','\'','0'};
int line;
int idenTableNum=0;
int tokenNum=0;
int conTableNum=0;
int flag=1;
char *notation_addr;

/**������ϣ��**/
HashNode *HashList[26];

/**����������ָ��**/
Token *head_token,*tail_token;  //�Ǵ�ͷ���ġ�����
IdenTable idenTableArray[VTSIZE];
ConTable_token conTableArray[CTSIZE];

int ClearBlank(void);
void dealinclude(void);
void dealdefine(void);
void dealundef(void);
void dealifdef(void);  //#ifdef
void dealifndef(void);  //#ifndef
void dealelse(void);  //#else
void dealendif(void);  //#endif
void deal_isnot_macro(void);  //���Ǻ궨��
int isargument(void);
int is_define_over(int i);
int count_argu(int i,char *argu_list);
ReplaceNode *creat_replace_string(int i,char *argu_list);
int is_in_list(char ch,char *argu_list);
void add_in(char ch,char *string_section);
int in_contable(char *name);
int in_contable_non(char *name);
int is_in_contable(char *name);
int is_in_contable_non(char *name);
int is_alpha(char ch);
int is_digit(char ch);
int is_space(char ch);

void dealalpha(void);
void dealdigit(void);
void dealNotation(void);
int isBorder(char ch);
int isinstruct(void);
void dealBorder(int ret);
void dealChar(void);
int isin_idenTable(char *word);
int isKeyword(char *word);
int isin_ConTable(char *word);
void dealinstruct(int ret);
void dealstring(void);
void shuchu(void);

int main(int argc,char **argv)
{
	int flag_include;
    int i,j;
    int odd_or_even;
    char in_file_name[40],out_file_name[43];
    char judge_include[15];  //����װ��#include <
	char defines[][10]={"#define","#undef","#ifdef","#ifndef","#else","#endif"};
	
	/**�����޲κ�ʹ��κ������**/
    head_non=(ConTable_non *)malloc(sizeof(ConTable_non));
    head=(ConTable *)malloc(sizeof(ConTable));
    tail_non=(ConTable_non *)malloc(sizeof(ConTable_non));
    tail=(ConTable *)malloc(sizeof(ConTable));
    head_non->next_non=(ConTable_non *)malloc(sizeof(ConTable_non));  //��ͷ��㣬����ɾ��
    head->next=(ConTable *)malloc(sizeof(ConTable));
    tail_non=head_non->next_non;
    tail=head->next;
    tail->next=NULL;
    tail_non->next_non=NULL;
	
	strcpy(in_file_name,argv[1]);
    in=fopen(in_file_name,"r");
    if(in==NULL)
	{
        printf("cannot open file %s!\n",in_file_name);
        return -1;
    }
    strcpy(out_file_name,"pre");
    strcat(out_file_name,in_file_name);
    out=fopen(out_file_name,"w");
    if(out==NULL)
	{
        printf("cannot open file %s!\n",out_file_name);
        return -1;
    }
	
    /**��ʼ�滻include���**/
    for(flag_include=1,odd_or_even=0;flag_include;odd_or_even=(odd_or_even+1)%2)  //odd_or_even��ֵΪ0��1
	{
		/**ͨ���������������ļ��Ľ�ɫת������ɶ��ɨ��**/
        flag_include=0;
        if(!odd_or_even){  //ͨ��odd_or_even�����ƴ��ĸ��ļ�������ȡ
            in=fopen(in_file_name,"r");
            if(in==NULL){
                printf("cannot open file %s!\n",in_file_name);
                return -1;
            }
            out=fopen(out_file_name,"w");
            if(out==NULL){
                printf("cannot open file %s!\n",out_file_name);
                return -1;
            }
        }
        else{
            in=fopen(out_file_name,"r");
            if(in==NULL){
                printf("cannot open file %s!\n",out_file_name);
                return -1;
            }
            out=fopen(in_file_name,"w");
            if(out==NULL){
                printf("cannot open file %s!\n",in_file_name);
                return -1;
            }
        }

		while(fgets(buf, LINESIZE, in)!=NULL)
		{
			start = 0;  //start����ֵ
			if(ClearBlank() == -1){
			/**��ǰ���ǿ���**/
				fputc('\n', out);  //��ǰ���д������
				continue;
			}
			/**��ǰ�в��ǿ���**/
			strncpy(judge_include,buf+start,10);
			judge_include[10]='\0';
		    if(!(strcmp(judge_include,"#include <")) || !(strcmp(judge_include,"#include \""))){
			 /**��incude���**/
				flag_include=1;
				dealinclude();
			}
			else{
				 fputs(buf,out);
			}
		}
		fclose(in);
		fclose(out);
    }
    /**include����滻���**/
	
    /**��ǰ��pre1.txt�д��include����֮����ļ��������������ļ�����һ��**/
	/**��ʼ����궨��**/
    in=fopen(in_file_name,"r");
    if(in==NULL){
        printf("cannot open file %s!\n",in_file_name);
        return -1;
    }
    out=fopen(out_file_name,"w");
    if(out==NULL){
        printf("cannot open file %s!\n",out_file_name);
        return -1;
    }
	
	while(fgets(buf, LINESIZE, in)!=NULL)
	{  //�������벻����#elif����Ϊ�������ʽ�Ƚ��鷳������
		for(start=0;buf[start] == ' ' || buf[start] == '\t';start++) ;
		strncpy(judge[0],buf+start,7);  //#define
		judge[0][7]='\0';
		strncpy(judge[1],buf+start,6);  //#undef
		judge[1][6]='\0';
		strncpy(judge[2],buf+start,6);  //#ifdef
		judge[2][6]='\0';
		strncpy(judge[3],buf+start,7);  //#ifndef
		judge[3][7]='\0';
		strncpy(judge[4],buf+start,5);  //#else
		judge[4][5]='\0';
		strncpy(judge[5],buf+start,6);  //#endif
		judge[5][6]='\0';
		for(j=0;j<6;j++)
		{
			if(!strcmp(judge[j],defines[j]))
				break;
		}
		switch(j)
		{
			case 0:	dealdefine(); break;  //#define
			case 1:	dealundef(); break;  //#undef
			case 2: dealifdef(); break;  //#ifdef
			case 3: dealifndef(); break; //#ifndef
			case 4: dealelse(); break; //#else
			case 5: dealendif(); break; //#endif
			default: deal_isnot_macro(); break;
		}
	}
    fclose(in);
    fclose(out);
	/**�괦�����**/
/**Ԥ���봦�����**/

	/**����token����**/
	head_token=(Token *)malloc(sizeof(Token));
	tail_token=(Token *)malloc(sizeof(Token));
	tail_token=head_token;
	tail_token->next = NULL;

	/**Ϊ�ؼ��ֽ�����ϣ����������ַ�������ַ��ͻ**/
	HashNode *hash_p,*hashtemp,*head_token;
	hash_p=(HashNode *)malloc(sizeof(HashNode));
	for(i=0;i<26;i++)
	{ //��ʼ��Ϊ��ָ��
		HashList[i] = NULL;
	}
	for(i=0;i<32;i++)
	{
		j=keywordList[i][0]-'a';
		if(HashList[j] == NULL)
		{ //��������
			head_token=(HashNode *)malloc(sizeof(HashNode));
			head_token->next=NULL;
			strcpy(head_token->name,keywordList[i]);
			head_token->times=0;
			HashList[j]=head_token;
		}
		else
		{
			for(hash_p = HashList[j];hash_p->next != NULL;hash_p=hash_p->next) ;
			hash_p->next=(HashNode *)malloc(sizeof(HashNode));
			strcpy(hash_p->next->name,keywordList[i]);
			hash_p->next->times=0;
			hash_p->next->next=NULL;
		}
	}
	/**��ϣ�������**/

	line = 1;
    char in_name[30],out_name[30];
    int ret;  //��Ϊ�������ص��н�
	strcpy(in_name,out_file_name);
	strcpy(out_name,"out.txt");
    in=fopen(in_name,"r");
    if(in == NULL){
        printf("cannot open file %s!\n",in_name);
        return -1;
    }
    out=fopen(out_name,"w");
    if(out == NULL){
        printf("cannot open file %s!\n",out_name);
        return -1;
    }

    /**��ʼ���дʷ�����**/
    char ch;
    while(fgets(buf,LINESIZE,in))
    {
        start=0;
        for(;buf[start]!= '\n' && flag;)  //flag�����ж�ע��
        {
            while(is_space(buf[start]))
            {
                start++;
            }  //ȥ���м����Ŀհ��ַ�
            ch=buf[start];
            if(is_alpha(ch) || ch == '_')  //��ĸ����ʶ�����ؼ���
                dealalpha();
            else if(is_digit(ch))  //���֣����ֳ���
                dealdigit();
            else if(ch== '/')  //����ע��
            {
                dealNotation();
            }
            else if(ret=isBorder(ch) != -1)  //�ָ���
            {
                dealBorder(ret);
            }
            else if(ch=='\'')
            {
                dealChar();  //�ַ�����
            }
            else if(ch == '"')
            {
                dealstring();  //�ַ���
            }
            else if((ret=isinstruct()) != -1)  //������
            {
                dealinstruct(ret);
            }
            else // not available start
            {
                printf("cannot analyse %c in line %d\n",ch,line);
				start++;
            }
        }
        line++;
		flag = 1;
    }
    shuchu();  //���ļ����
	fclose(in);
	fclose(out);
/**�ʷ��������**/

	system("pause");
	return 0;
}

/**��λ����ǰ�������ĵ�һ���ǿհ��ַ�**/
int ClearBlank(void)
{
    char ch;
    ch=buf[start];
    for(;ch ==' ' || ch =='\n' || ch =='\t';){
        if(ch=='\n'){
            return -1;  //��ǰ���ǿ���
        }
        ch=buf[++start];
    }
    return 0;
}

/**����include���**/
void dealinclude(void)
{
/**ֻ���Ǳ�׼��ͷ�ļ��������û��Զ����ͷ�ļ�Ҳһ��**/
    char temp_file_name[20];  //�洢ͷ�ļ�����
    char head_file_name[100];
    char buf_include[LINESIZE];
    int i;
    start=start+10;
    for(i=0;buf[start]!='>' && buf[start]!='\"';start++,i++)
	{
        temp_file_name[i]=buf[start];
    }
    temp_file_name[i]='\0';
    //ͷ�ļ����ѱ�����
    if(strstr(temp_file_name,"/")!=NULL)  //��include����Ŀ¼����
	{
        if(strcmp(temp_file_name,"machine/ansi.h")==0 || strcmp(temp_file_name,"sys/_types.h")==0)
		{
            strcpy(head_file_name,"head/");
            strcat(head_file_name,temp_file_name);
        }
        else
		{
            strcpy(head_file_name,"D:/��װ�����/CodeBlocks/MinGW/include/");
            strcat(head_file_name,temp_file_name);
        }
    }
    else
	{
        strcpy(head_file_name,"head/");
        strcat(head_file_name,temp_file_name);
    }
    FILE *op;
    op=fopen(head_file_name,"r");
    if(op==NULL)
	{
        printf("cannot open the head file %s!\n",head_file_name);
    }
    while(fgets(buf_include, LINESIZE, op)!=NULL)
	{
        fputs(buf_include,out);
    }
    fclose(op);
}

/**����define**/
void dealdefine(void)
{
	define_addr = strstr(buf,"#define ");
    int i,j;
    ConTable_non *temp_non;
    ConTable *temp;
    int a;
    a=isargument();
    if(a==0)  //�޲κ�
	{
        temp_non=(ConTable_non *)malloc(sizeof(ConTable_non));
        for(i=8,j=0;define_addr[i]!=' ' ;i++,j++)
		{
            temp_non->name_non[j]=define_addr[i];  //�����������
        }
		temp_non->name_non[j]='\0';
    /**����Ϊ�������������ʱdefine�ͺ���֮��ֻ�ܸ�һ���ո�**/
        i++;
        for(j=0;;i++,j++)
		{
            if(define_addr[i]!='\n')
			{
                temp_non->replace_non[j]=define_addr[i];
            }
            else if(is_define_over(i) == 1)  //����define���Ľ�β
			{
                temp_non->replace_non[j]='\0';
                break;
            }
            else{
                temp_non->replace_non[j]=define_addr[i];
            }
        }  //������ַ����������
        tail_non->next_non=temp_non;
        tail_non=temp_non;
        tail_non->next_non=NULL;
    }  //�޲κ�����������
    else  //���κ�
	{
        temp=(ConTable *)malloc(sizeof(ConTable));
        //�Բ����б���г�ʼ��
        for(j=0;j<20;j++)
		{
            temp->argu_list[j]='\0';
        }
        for(i=8,j=0;define_addr[i]!='(' ;i++,j++)
		{
            temp->name[j]=define_addr[i];  //�����������
        }
		temp->name[j]='\0';
        temp->argu_count=count_argu(i,temp->argu_list);
        temp->replace=(ReplaceNode *)malloc(sizeof(ReplaceNode));
        temp->replace=creat_replace_string(i,temp->argu_list);
        tail->next=temp;
        tail=temp;
        tail->next=NULL;
    }  //���κ�����������
}

/**����undef���**/
void dealundef(void)
{
	define_addr = strstr(buf,"#undef ");
    char temp_name[100];  //����
    int i,j;
    for(i=7,j=0;!(isspace(define_addr[i]));i++,j++){
        temp_name[j]=define_addr[i];
    }
    temp_name[j]='\0';
    //�����������
    if(in_contable_non(temp_name) == 0){  //�����޲κ꣬������ɾ��
        if(in_contable(temp_name) == 0){  //Ҳ���Ǵ��κ꣬Ҳ������ɾ��
            printf("undefined HONG!\n");  //����������ֻ�����ʾ
        }
    }
}

/**����#ifdef**/
void dealifdef(void)
{
	int i;
	ConTable *pre,*now;  //�����жϺ��Ƿ��Ѷ���
	ConTable_non *pre_non,*now_non;
	flag_def++;
	start=start+6;
	for(;isspace(buf[start]);start++) ;
	for(i=0;!isspace(buf[start]);start++,i++)
	{
		judge[2][i]=buf[start];
	}
	judge[2][i]='\0';
	
	/**��ʼ����Ƿ����Ѿ�������޲κ�**/
	pre_non=head_non;
	now_non=head_non->next_non;
	for(;now_non!= NULL; pre_non=now_non,now_non=now_non->next_non){
		if(strcmp(judge[2],now_non->name_non)!= 0) ;
		else{
			break;
		}
	}
	pre=head;
	now=head->next;
	for(;now!= NULL; pre=now,now=now->next){
		if(strcmp(judge[2],now->name)!= 0) ;
		else{
			break;
		}
	}
	if(now_non!= NULL || now!= NULL)
	{  //���Ѿ�����ĺ�
		flag_ifdef=1;
	}
	else  //δ���ǵ�#elif
	{  //��ǰ��δ������
		flag_ifdef=0;
	}
}

/**����#ifndef**/
void dealifndef(void)
{
	int i;
	ConTable *pre,*now;  //�����жϺ��Ƿ��Ѷ���
	ConTable_non *pre_non,*now_non;
	flag_def++;
	start=start+7;
	for(;isspace(buf[start]);start++) ;
	for(i=0;!isspace(buf[start]);start++,i++)
	{
		judge[3][i]=buf[start];
	}
	judge[3][i]='\0';
	
	/**��ʼ����Ƿ����Ѿ�����ĺ�**/
	pre_non=head_non;
	now_non=head_non->next_non;
	for(;now_non!= NULL; pre_non=now_non,now_non=now_non->next_non){
		if(strcmp(judge[3],now_non->name_non)!= 0) ;
		else{
			break;
		}
	}
	pre=head;
	now=head->next;
	for(;now!= NULL; pre=now,now=now->next){
		if(strcmp(judge[3],now->name)!= 0) ;
		else{
			break;
		}
	}
	/**������**/
	
	if(now_non== NULL && now== NULL)
	{  //��ǰ��δ������
		flag_ifdef=1;
	}
	else  //δ���ǵ�#elif
	{  //�Ǳ�����ĺ�
		flag_ifdef=0;
	}
}

/**����#else**/
void dealelse(void)
{
	flag_ifdef=(flag_ifdef+1)%2;  //��flag_ifdef 0��Ϊ1,1��Ϊ0
}

/**����#endif**/
void dealendif(void)
{
	flag_def--;
	flag_ifdef = 1;
}

/**�����Ǻ궨������**/
void deal_isnot_macro(void)
{
	if(flag_ifdef == 0)  //ֱ�Ӻ��Ե�ǰ���
	{
			return;
	}
  //���봦�����Ҳ��������Ĵ������
	start = 0;
	if(ClearBlank() == -1)  //��ǰ���ǿ���
	{
		/**������ѿ���ɾ������**/
		return;
	}
	if(buf[start] == '/' && buf[start+1] == '/')  //�����ע��
	{
		fputs(buf+start,out);
		return ;
	}
	char string_temp[100];  //�滻defineʱ����Ŵ�buf�ж�ȡ�ĵ����ַ���
	int j;
	while(buf[start] != '\0')
	{
		for(j=0;is_alpha(buf[start]) || is_digit(buf[start]) || buf[start] == '_';j++,start++){  /**ע�⣡�������������������ѭ���ķ�ʽ����Ҫ��ȶ**/
			string_temp[j]=buf[start];
		}  //һ���ַ����������
		/**��ǰbuf[start] == '('**/
		string_temp[j]='\0';
		if(is_in_contable_non(string_temp)){  //�޲κ��滻
			fputc(buf[start],out);
			start++;
		}
		else if(is_in_contable(string_temp)){  //���κ��滻
			start++;
			fputc(buf[start],out);
			start++;
		}
		else
		{
			fputs(string_temp,out);  //ԭ�����
			fputc(buf[start],out);
			start++;
		}
	}
}

/**�ж�define����Ƿ����**/
int is_define_over(int i)
{
    /**��ǰ״̬��define_addr[i]==' '**/
    char temp;
    temp=define_addr[i];
    for(;temp!= '\n';)
	{
        if(temp == ' ') ;
        else if(temp != '/')
            return 0;
        else if(define_addr[i+1] == '*' || define_addr[i+1] == '/')  //��ע��
            return 1;
        else
            return 0;
        temp=define_addr[++i];
    }
    if(temp == '\n')
        return 1;
    else
        return 0;
}

/**�ж��Ƿ����޲κ�**/
int isargument(void)
{
    char temp[100];
    int i;
    for(i=0;;i++)
	{
        temp[i]=define_addr[i+8];
        if(temp[i]==' ' || temp[i]=='('){
            if(temp[i]=='(')
                return 1;
            else
                return 0;
        }
    }
}

/**������κ�����ĸ��������Բ����б���и�ֵ**/
/**�����б��[1]��ʼ**/
int count_argu(int i,char *argu_list)
{
    int a;
    /**��ǰ״̬��define_addr[i]== '('**/
    i++;
    for(a=0;define_addr[i]!= ')';i++){
        if(define_addr[i]!= ','){
            a++;
            argu_list[a]=define_addr[i];
        }
    }
    return a;
}

/**�������κ������ַ���**/
ReplaceNode *creat_replace_string(int i,char *argu_list)
{
    int ret;
    int j,k;
    char temp[10][10];
    /**��ǰ״̬��define_addr[i] == '('**/
    ReplaceNode *loc_head,*loc_tail;
    i++;
    for(;define_addr[i]!= ')';i++) ;
    /**��ǰ״̬��define_addr[i] == ')'**/
    i++;
    for(;define_addr[i] == ' ';i++) ;
    loc_head=(ReplaceNode *)malloc(sizeof(ReplaceNode));  //��������ַ�������ָ�룬����Ҫͷ���
    loc_tail=loc_head;
    j=0;
	k=0;
    for(;;i++)
    {
        if(define_addr[i] == '\n')
		{
				temp[k][j]='\0';
				strcpy(loc_tail->string_section,temp[k]);
				loc_tail->next_replace=NULL;
                break;
        }
        if(define_addr[i]>='a' && define_addr[i]<= 'z' || define_addr[i]>='A' && define_addr[i]<= 'Z')
		{
            ret=is_in_list(define_addr[i],argu_list);
            if(ret){  //�ڲ����б���
                temp[k][j]='\0';
				strcpy(loc_tail->string_section,temp[k]);
				k++;
                j=0;
                loc_tail->next_replace=(ReplaceNode *)malloc(sizeof(ReplaceNode));
                loc_tail=loc_tail->next_replace;  //�Ƚ����ǲ����Ӵ�����
                loc_tail->id_flag=ret;
				strcpy(loc_tail->string_section,"");
                loc_tail->next_replace=(ReplaceNode *)malloc(sizeof(ReplaceNode));
                loc_tail=loc_tail->next_replace;
                continue;
            }
            else
            {
                loc_tail->id_flag=0;
                temp[k][j]=define_addr[i];
                j++;
            }
        }
		else
		{
			//���ڲ����б���
			loc_tail->id_flag=0;
			temp[k][j]=define_addr[i];
			j++;
		}
    }
    return loc_head;
}

/**�ж���ĸ�Ƿ��ڵ�ǰ���κ�Ĳ����б���**/
int is_in_list(char ch,char *argu_list)
{
    int i;
    for(i=1;argu_list[i]!= '\0';i++){
        if(ch == argu_list[i]){
            return i;
        }
    }
    return 0;
}

/**�Ƿ������޲κ꣬������ɾ��**/
int in_contable_non(char *name)
{
    ConTable_non *pre,*now;
    pre=head_non;
    now=head_non->next_non;
    for(;now!= NULL; pre=now,now=now->next_non){
        if(strcmp(name,now->name_non)!= 0) ;
        else{
            pre->next_non=now->next_non;
            free(now);
            return 1;
        }
    }
    return 0;
}

/**�Ƿ����ڴ��κ�,������ɾ��**/
int in_contable(char *name)
{
    ConTable *pre,*now;
    pre=head;
    now=head->next;
    for(;now!= NULL; pre=now,now=now->next){
        if(strcmp(name,now->name)!= 0) ;
        else{
            pre->next=now->next;
            free(now);
            return 1;
        }
    }
    return 0;
}

/**�ж��Ƿ����޲κ��б��У��������滻**/
int is_in_contable_non(char *name)
{
    ConTable_non *pre,*now;
    pre=head_non;
    now=head_non->next_non;
    for(;now!= NULL; pre=now,now=now->next_non){
        if(strcmp(name,now->name_non)!= 0) ;
        else{
            fputs(now->replace_non,out);  //����滻���ַ���
            //fprintf(out," ");  //����ո�
            return 1;
        }
    }
    return 0;
}

/**�ж��Ƿ��ڴ��κ��б��У��������滻**/
int is_in_contable(char *name)
{   
    /**��Ҫע�⣬���ô��κ�ʱ���������ܱ�������Ϊһ������**/
    ConTable *pre,*now;
    char charge[100];
	char **temp;
    charge[0]='\0';
    ReplaceNode *tempnode;
    int count;
    int i,j,flag_temp;
    flag_temp=1;  //�����������ŵĶ�Ӧ��ϵ
    pre=head;
    now=head->next;
    for(;now!= NULL; pre=now,now=now->next)
    {
        if(strcmp(name,now->name)!= 0) ;
        else  //�Ǵ��κ�
        {
            count=now->argu_count;
			//count = 2;
			temp=(char **)malloc(sizeof(char *)*count);
			for(i=0;i<count;i++)
			{
				temp[i]=(char *)malloc(sizeof(char)*20);
			}
			/**��ǰbuf[start] == '('**/
            for(i=0;i < count;i++)  //i�����жϵ�ǰ�ǵڼ�������
            { //ѭ��ȡ�������
                start++;
                for(j=0;buf[start]!= ',';start++,j++)
                { //ѭ��ȡһ�������Ķ���ַ�
                    if(i == count-1)  //��������һ�������Ļ����ڴ˴�break
                    {
                        if(flag_temp == 1 && buf[start] == ')')
                            break;
                    }
					if(buf[start] == '(')
                        flag_temp++;
                    if(buf[start] == ')')
                        flag_temp--;
					temp[i][j]=buf[start];
                }
				temp[i][j]='\0';
            }
            /**���������ϣ���ʼ�滻����**/
            tempnode = now->replace;
            for(;tempnode != NULL;tempnode=tempnode->next_replace)
            {
                if(tempnode->id_flag == 0)
                {
                    strcat(charge,tempnode->string_section);
                }
                else
                {
                    strcat(charge,temp[tempnode->id_flag-1]);
                }
            }
            /**�滻������ϣ�׼�����ļ������**/
            fputs(charge,out);  //����滻���ַ���
            return 1;
        }
    }
    return 0;
}

/**�ж��Ƿ�����ĸ**/
int is_alpha(char ch)
{
	if(ch>='a' && ch<= 'z' || ch>= 'A' && ch<= 'Z')
		return 1;
	else
		return 0;
}

/**�ж��Ƿ�������**/
int is_digit(char ch)
{
	if(ch>='0' && ch<= '9')
		return 1;
	else
		return 0;
}

/**�ж��Ƿ��ǿհ׷�**/
int is_space(char ch)
{ //ֻ���ǵ��ո���Ʊ������Ϊ��ʱֻ�õ�������������
	if(ch == ' ' || ch == '\t')
		return 1;
	else
		return 0;
}

/**�����ʶ�����ؼ���**/
void dealalpha(void)
{
    int symbol;
    int id;
    char word[100];
    Token *token;
	token=(Token *)malloc(sizeof(Token));
    IdenTable idenTable;
    int i;
    token->x=line;
    token->y=start;
	for(i=0;is_digit(buf[start]) || is_alpha(buf[start]) || buf[start] == '_' || buf[start] == '#';i++,start++)
    {
        word[i]=buf[start];
    }  //��ʱ��buf[start] == ' '
    word[i] = '\0';

    strcpy(token->name,word);
    symbol = isKeyword(word);
    /**���ǹؼ��֣��Ǳ�ʶ��**/
    if(symbol == -1)
    {
        token->number = IDENTIFY;
        id=isin_idenTable(word);
        if(id== -1)
        {
            idenTable.id=idenTableNum;
            strcpy(idenTable.name,word);
            idenTable.times=1;
            idenTableArray[idenTableNum] = idenTable;
            token->times=idenTableArray[idenTableNum].times;
            idenTableNum++;
        }
        else
        {
            idenTableArray[id].times++;
            token->times=idenTableArray[id].times;
        }
    }
    /**�ǹؼ���**/
    else
    {
        token->number= KEYWORD;
		token->times= symbol;
    }
	tail_token->next=token;
	tail_token=tail_token->next;
	tail_token->next=NULL;
    tokenNum++;
}

/**�ж��Ƿ��Ѿ��ڱ�ʶ���ı���**/
int isin_idenTable(char *word)
{
    int i;
    for(i=0;i<idenTableNum;i++)
    {
        if(!strcmp(idenTableArray[i].name,word))
        {
            return i;
        }
    }
    return -1;
}

/**�ж��Ƿ��ǹؼ��֣����ǣ����س��ֵĴ���**/
int isKeyword(char *word)
{
	int j;
	HashNode *hashp;
	j=word[0]-'a';
	for(hashp=HashList[j];hashp!= NULL; hashp=hashp->next)
    {
		if(!strcmp(hashp->name,word))
        {
			hashp->times++;
			return hashp->times;
        }
    }
    return -1;
}

/**�������ֳ���**/
void dealdigit(void)
{
    int id;
	int flag_f=0;
    char word[100];
    Token *token;
	token=(Token *)malloc(sizeof(Token));
    ConTable_token conTable;
    int i;
    token->x=line;
    token->y=start;											//+  -  ֻ����E�ĺ���
	i=0;
	if(buf[start] == '0' && (buf[start+1] == 'X' || buf[start+1] == 'x'))  //ʮ������
	{
		word[i++]=buf[start++];
		word[i++]=buf[start++];
		for(;is_digit(buf[start]) || (buf[start]>='A' && buf[start]<='F') || (buf[start]>='a' && buf[start]<='f');i++,start++)
		{
			word[i]=buf[start];
		}
	}
	else if(buf[start] == '0') //�˽���
	{
		word[i++]=buf[start++];
		for(;is_digit(buf[start]);i++,start++)
		{
			word[i]=buf[start];
		}
	}
	else
	{
		for(;is_digit(buf[start]) || buf[start] == '.' || buf[start] == 'E' || buf[start] == 'e' || (buf[start] == '+' && (buf[start-1] == 'E' || buf[start-1] == 'e')) || (buf[start] == '-' && (buf[start-1] == 'E' || buf[start-1] == 'e'));i++,start++)
		{
			if(buf[start] == '.')
				flag_f = 0;
			 word[i]=buf[start];
		}
		if(buf[start] == 'f' || buf[start] == 'F'){  //��F ��f��׺
			word[i]=buf[start];
			i++;
			start++;
		}
	}
    word[i] = '\0';
    strcpy(token->name,word);
    token->number=CONSTANT;
    id = isin_ConTable(word);
    if(id==-1)
    { /**���ڳ�������¼���**/
        conTable.id = conTableNum;  //�����տ�ʼ��ô�ͱ��1�ˡ�������Ӧ����0�𡣡�����
        strcpy(conTable.name, word);
        conTable.times=1;
        conTableArray[conTableNum] = conTable;
        token->times=conTableArray[conTableNum].times;
        conTableNum++;
    }
    else
    {
        conTableArray[id].times++;
        token->times=conTableArray[id].times;
    }
    tail_token->next=token;
	tail_token=tail_token->next;
	tail_token->next=NULL;
    tokenNum++;
}

/**�ж��Ƿ��ڳ�������**/
int isin_ConTable(char *word)
{
    int i;
    for(i=0;i<conTableNum;i++)
    {
        if(!strcmp(conTableArray[i].name,word))
        {
            return i;
        }
    }
    return -1;
}

/**��������ע��**/
void dealNotation(void)
{ /**��ǰ��buf[start] == '/'**/
    char ch;
    int i;
    int notationLen=0;
    Token *token;
	token=(Token *)malloc(sizeof(Token));
    ch = buf[start+1];
    /**���Ŵ���**/
    if(ch!='/'&&ch!='*')
    {
        strcpy(token->name,"/");
        token->number = OPERATOR;
        instructtimes[3]++;
        token->times = instructtimes[3];
        token->x = line;
        token->y = start;
        tail_token->next=token;
		tail_token=tail_token->next;
		tail_token->next=NULL;
        tokenNum++;
        start++;
        return ;
    }
    else if(ch=='/')
    {
        flag=0;  //��Ҫ���¿�ʼһ��
        return ;
    }
    else
    {
        for(i=start+2;; i++)
        { /**��ǰ��buf[i] == ע�ͺ�ĵ�һ���ַ�**/

			if((notation_addr = strstr(buf,"*/"))!= NULL)  //��ǰ����ע�͵Ľ�β���
			{
				if(notation_addr <= buf+start+1)
				{  //�����ǵ�ǰע�͵Ľ�β�����Ի�Ҫ����ִ����ȥ

				}
				else
				{
					for(;;start++)
					{
						if(buf[start] == '*' && buf[start+1] == '/')
						{ //��ʱ��buf[start] == '*'
							start = start+2;
							return ;  //ע�ʹ�����ϣ�����
						//��ǰ��buf[start] == ע�ͽ�����ĵ�һ���ַ�**/
						}
						else ;
					}
				}
			}
			fgets(buf, LINESIZE, in);
            line++;  //��Ȼע�ͱ������ˣ�����ע������������ı仯��Ȼ����¼��
            start = 0;
        }
    }
}

/**�ж��Ƿ��Ƿָ���**/
int isBorder(char ch)
{
    int i;
    for(i=0; borderList[i][0]; i++)
    {
        if(ch==borderList[i][0])
        {
            return i;
        }
    }
    return -1;
}

/**����ָ���**/
void dealBorder(int ret)
{
    Token *token;
	token=(Token *)malloc(sizeof(Token));
    token->name[0]=buf[start];
    token->name[1]='\0';
    token->number=BORDER;
    bordertimes[ret]++;
    token->times=bordertimes[ret];
    token->x = line;
    token->y = start;
    tail_token->next=token;
	tail_token=tail_token->next;
	tail_token->next=NULL;
    tokenNum++;
    start++;
}

/**�ж��Ƿ��ǲ�����**/
int isinstruct(void)
{
    char s[3];
    int ret;
    int i;
    s[0] = buf[start];
    s[1] = buf[start+1];
    s[2] = '\0';
    if (s[1]=='+' || s[1]=='-' || s[1]=='=' || s[1]=='&' || s[1]=='|' || s[1]=='>' || s[1]=='<')
    { /**����˫���**/
        switch(s[1]){
            case  '+':  ret=18; break;
            case  '-':  ret=19; break;
            case  '&':  ret=25; break;
            case  '|':  ret=26; break;
            case  '=':  ret=0; break;
            case  '>':  ret=32; break;
            case  '<':  ret=33; break;
        }
        if(!ret)  //�ԵȺŽ�β�Ĳ�����
        {
            switch(s[0]){
                case  '=':  ret=20;
                case  ':':  ret=21;
                case  '>':  ret=22;
                case  '<':  ret=23;
                case  '!':  ret=24;
                case  '+':  ret=27;
                case  '-':  ret=28;
                case  '*':  ret=29;
                case  '/':  ret=30;
            }
        }
        return ret;
    }
    /**�жϵ����**/
    for(i=0; instructList[i][0]; i++)
    {
        if(buf[start]==instructList[i][0])
        {
            return i;
        }
    }
    return -1;
}

/**���������**/
void dealinstruct(int ret)
{
    Token *token;
	token=(Token *)malloc(sizeof(Token));
    token->y = start;
    if((ret>=18 && ret<=30) || ret == 32 || ret == 33)
    {
        token->name[0]=buf[start];
        token->name[1]=buf[start+1];
        token->name[2]='\0';
        start=start+2;
    }
    else
    {
        token->name[0]=buf[start];
        token->name[1]='\0';
        start++;
    }
    token->number=OPERATOR;
    instructtimes[ret]++;
    token->times=instructtimes[ret];
    token->x = line;
	tail_token->next=token;
	tail_token=tail_token->next;
	tail_token->next=NULL;
    tokenNum++;
}

/**�����ַ������ַ�����**/
void dealChar(void)
{
    Token *token;
	token=(Token *)malloc(sizeof(Token));
    ConTable_token conTable;
    int id;
    int j;
    char word[100];
    word[0] = buf[start];
    if(buf[start+1]=='\\')//ת���ַ�
    {
        for(j=0; j<12; j++)
        {
            if(buf[start+2]==changeList[j])
            {
                word[1] = '\\';
                word[2] = buf[start+2];
                word[3] = '\'';
                word[4] = '\0';
                strcpy(token->name, word);
                token->number=CONSTANT;
                changetimes[j]++;
                token->times=changetimes[j];
                token->x=line;
                token->y=start;
                tail_token->next=token;
				tail_token=tail_token->next;
				tail_token->next=NULL;
                tokenNum++;
                start = start+4;
                return;
            }
        }
        /**error: ת���ַ����Ϸ�**/
        if(j==12)
        {
            printf("wrong change char in line %d!\n", line);
        }
    }
    else if(buf[start+2] != '\'')
    {
        printf("wrong const char in line %d!\n", line);
        for(start = start+2; buf[start]!='\''; start++);
        start++ ;
        return;
    }
    else
    {
        word[1] = buf[start+1];
        word[2] = '\'';
        word[3] = '\0';
        strcpy(token->name,word);
        token->number=CONSTANT;
        token->x=line;
        token->y=start;
        id = isin_ConTable(word);
        /**���ڳ�������¼���**/
        if(id==-1)
        {
            conTable.id = conTableNum;
            strcpy(conTable.name, word);
            conTable.times=1;
            conTableArray[conTableNum] = conTable;
            token->times=conTableArray[conTableNum].times;
            conTableNum++;
        }
        else
        {
            conTableArray[id].times++;
            token->times=idenTableArray[id].times;
        }
        tail_token->next=token;
		tail_token=tail_token->next;
		tail_token->next=NULL;
        tokenNum++;
        start = start+3;
        return;
    }
}

/**�����ַ�������**/
void dealstring(void)
{
    int j;
    Token *token1,*token2,*token3;
	token1=(Token *)malloc(sizeof(Token));
	token2=(Token *)malloc(sizeof(Token));
	token3=(Token *)malloc(sizeof(Token));
    ConTable_token conTable;
    int id;
    char word[100];

	token1->name[0]='"';
	token1->name[1]='\0';
	token1->number=OPERATOR;
	instructtimes[37]++;
	token1->times=instructtimes[37];
	token1->x=line;
	token1->y=start;
	tail_token->next=token1;
	tail_token=tail_token->next;
	tail_token->next=NULL;
    tokenNum++;
	start++;

	/**���ַ����е�һ������ " ���ַ���ʼ**/
    word[0]=buf[start];
    token2->y=start;
    start++;
        for(j=1; !(buf[start]=='"' && buf[start-1] !='\\'); start++,j++)
        {
            word[j] = buf[start];
        }
		word[j]='\0';
        strcpy(token2->name,word);
        token2->number=STRING;
        token2->x=line;
        id = isin_ConTable(word);
        /**���ڳ�������**/
        if(id==-1)
        {
            conTable.id = conTableNum;
            strcpy(conTable.name, word);
            conTable.times=1;
            conTableArray[conTableNum] = conTable;
            token2->times=conTableArray[conTableNum].times;
            conTableNum++;
        }
    else
    {
        conTableArray[id].times++;
        token2->times=conTableArray[id].times;
    }
        tail_token->next=token2;
		tail_token=tail_token->next;
		tail_token->next=NULL;
        tokenNum++;

	/**�����β�� " **/
	token3->name[0]='"';
	token3->name[1]='\0';
	token3->number=OPERATOR;
	instructtimes[37]++;
	token3->times=instructtimes[37];
	token3->x=line;
	token3->y=start;
	tail_token->next=token3;
	tail_token=tail_token->next;
	tail_token->next=NULL;
    tokenNum++;
	start++;
}

/**��out.txt�ļ����**/
void shuchu(void)
{
	int i;
	Token *tp;
	tp=head_token->next;
    for(i=1;tp!=NULL;tp=tp->next,i++)
    { /**ע�⣺��Ϊ������������[0]��ʼ�ģ�����������к�ʱ��Ҫ+1**/
        fprintf(out,"%-4d  %-4d  %-30s  %-4d  %-4d  %-4d\n",i,tp->number,tp->name,tp->x,tp->y+1,tp->times);
    }
}




