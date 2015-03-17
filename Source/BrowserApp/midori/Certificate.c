/*
   main function:
      get single certificate information
      output single certificate information
  author:luyue
*/

#include "Certificate.h"
#include "Certificate_Password.h"

#include "openssl/pkcs12.h"
#include "openssl/x509v3.h"
#include "openssl/pem.h"

static const char *exname[] = 
{
   "X509v3 Basic Constraints",
   "X509v3 Key Usage", 
   "X509v3 Certificate Policies",
   "X509v3 CRL Distribution Points",
   "X509v3 Extended Key Usage",
   "Authority Information Access",
   "X509v3 Authority Key Identifier",
   "Netscape Cert Type",
   "X509v3 Subject Key Identifier",
   "X509v3 Subject Alternative Name",
   "X509v3 Issuer Alternative Name",
   "Netscape Comment",
   "X509v3 Private Key Usage Period",
   "X509v3 Name Constraints",
   "X509v3 Policy Mappings"
};

static const BIT_STRING_BITNAME reason_flags[] = {
{0, "Unused", "unused"},
{1, "Key Compromise", "keyCompromise"},
{2, "CA Compromise", "CACompromise"},
{3, "Affiliation Changed", "affiliationChanged"},
{4, "Superseded", "superseded"},
{5, "Cessation Of Operation", "cessationOfOperation"},
{6, "Certificate Hold", "certificateHold"},
{7, "Privilege Withdrawn", "privilegeWithdrawn"},
{8, "AA Compromise", "AACompromise"},
{-1, NULL, NULL}
};

static const unsigned char char_type[] = {
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
120, 0, 1,40, 0, 0, 0,16,16,16, 0,25,25,16,16,16,
16,16,16,16,16,16,16,16,16,16,16, 9, 9,16, 9,16,
 0,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
16,16,16,16,16,16,16,16,16,16,16, 0, 1, 0, 0, 0,
 0,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
16,16,16,16,16,16,16,16,16,16,16, 0, 0, 0, 0, 2
};

static const signed char tag2nbyte[] = {
        -1, -1, -1, -1, -1,     /* 0-4 */
        -1, -1, -1, -1, -1,     /* 5-9 */
        -1, -1, 0, -1,          /* 10-13 */
        -1, -1, -1, -1,         /* 15-17 */
        -1, 1, 1,               /* 18-20 */
        -1, 1, 1, 1,            /* 21-24 */
        -1, 1, -1,              /* 25-27 */
        4, -1, 2                /* 28-30 */
};

static const char *mon[12]=
{
   "Jan","Feb","Mar","Apr","May","Jun",
   "Jul","Aug","Sep","Oct","Nov","Dec"
};

struct Certificate certificate;

//单个证书详细显示中证书字段指针
GtkTreeIter   iter_v;

//标记single-certificate.glade是否要打开
static glade_num;

//保存单个证书中证书链信息（至多３个）
char *ce_chain_value;
char *ce_chain_value1;
char *ce_chain_value2;

//证书管理器的授权中心中的证书名称
char *root_name;

//证书管理器您的证书中的证书名称
char *personal_name;

//证书管理器window
GtkWindow *manager_window;
GtkButton *manager_rootshow_button;
GtkButton *manager_rootexport_button;
GtkButton *manager_rootdelete_button;
GtkButton *personal_show_button; 
GtkButton *personal_export_button;
GtkButton *personal_delete_button;

//单个证书window
GtkWindow *certificate_window;
GtkBuilder *certificate_builder;
GtkWidget *tree1,*tree2,*tree3;
GtkTreeStore *tree_store1,*tree_store2,*tree_store3;
GtkCellRenderer *renderer1,*renderer2,*renderer3;
GtkWidget *scrolledwindow, *scrolledwindow1,*scrolledwindow2;

//弹出密码window
GtkWidget *popup_password_dialog;
GtkWidget* entry;
GtkWidget *personal_import_dialog;
PKCS12 *p12;
char *personal_certificate_path;
char *certificate_password;
char *export_personal_path;//导出时使用
//ASN1_Time to SIZE_T_Time
static char * asn1_gettime(ASN1_TIME* time,char *date)
{
   const char*str;
   int year,mon,day;

   str=(const char*) time->data;
   if (time->type==V_ASN1_UTCTIME) 
   {
      year=(str[0]-'0')*10+(str[1]-'0');
      if(year<50)
      {
         year+=2000;
      }
      else year+=1900;
      mon=((str[2]-'0')*10+(str[3]-'0'));
      day=(str[4]-'0')*10+(str[5]-'0');
      sprintf(date,"%d年%02d月%02d日",year,mon,day);
   }
   else if(time->type==V_ASN1_GENERALIZEDTIME) 
   {
      year=(str[0]-'0')*1000+(str[1]-'0')*100+(str[2]-'0')*10+(str[3]-'0');
      mon=(str[4]-'0')*10+(str[5]-'0');
      day=(str[6]-'0')*10+(str[7]-'0');
      sprintf(date,"%d年%02d月%02d日",year,mon,day);   
   }
   return date;
}

//X509_ALGOR* convert to char*
static void x509_algor( X509_ALGOR *sigalg,char **sig_alg)
{
    char buf[80]={0};
    int i;

    i=i2t_ASN1_OBJECT(buf,sizeof buf,sigalg->algorithm);
    if (i > 0)
    {
       *sig_alg = malloc(i + 1);
       i2t_ASN1_OBJECT(*sig_alg,i + 1,sigalg->algorithm);
    }
}

//ASN1_STRING* convert to char*
static void x509_asn1_string(ASN1_STRING *sig,char **signature) 
{
   unsigned char *s;
   int i, n;
   char tmpdata[4]={0};

   n=sig->length;
   s=sig->data;
   *signature = (char *)malloc(n*3+1+n/18);
   for (i=0; i<n; i++)
   {     
      sprintf(tmpdata,"%02x ",s[i]);
      if(i==0)
         strcpy(*signature,tmpdata);
      else
      {
         if ((i%18) == 0)
           strcat(*signature,"\n");  
         strcat(*signature,tmpdata);
      }
   }
}

static void update_buflen(const BIGNUM *b, size_t *pbuflen)
{
   size_t i;
   if (!b) return;
   if (*pbuflen < (i = (size_t)BN_num_bytes(b)))
      *pbuflen = i;
}

// RSA* convert to char*
static void x509_get_pkey_value(RSA *rsa,char **pkey_value)
{
   unsigned char *m=NULL;
   int mod_len = 0;
   size_t buf_len=0;
   char tmpdata[32]={0};
   int n,i;

   update_buflen(rsa->n, &buf_len);
   update_buflen(rsa->e, &buf_len);  
   m=(unsigned char *)malloc(buf_len+10);
   mod_len = BN_num_bits(rsa->n);
   m[0]=0;
   n=BN_bn2bin(rsa->n,&m[1]);
   *pkey_value = (char *)malloc(sizeof(tmpdata)*3+n*3+n/18);
   sprintf(tmpdata,"Private-Key: (%d bit)\n",mod_len);
   strcpy(*pkey_value,tmpdata);
   strcat(*pkey_value,"Modulus:\n");
   if (m[1] & 0x80)
      n++;
   else    
      m++;
   for (i=1; i<n; i++)
   {     
      sprintf(tmpdata,"%02x%s",m[i],((i+1) == n)?"\n":":");
      strcat(*pkey_value,tmpdata);
      if ((i%18) == 0)
        strcat(*pkey_value,"\n");  
   }
   free(m);
   m = NULL; 
   sprintf(tmpdata,"Exponent: %s%lu (%s0x%lx)",(BN_is_negative(rsa->e))?"-":"",
                        (unsigned long)(rsa->e)->d[0],(BN_is_negative(rsa->e))?"-":"",
                        (unsigned long)(rsa->e)->d[0]);
   strcat(*pkey_value,tmpdata);  
}

typedef int char_io(char *value, const void *buf, int len);

static int send_bio_char(char *value, const void *buf, int len)
{
   int i;
   if(!value) return 1;
   if(strlen(buf) < len)
      return 0;
   else if(strlen(buf) > len)
   {
      for(i=0;i<strlen(buf)-len;i++)
         strcat(value," ");       
   }
   strcat(value,buf);
   return 1;
}

static int do_indent(char_io *io_ch, char *value, int indent)
{
   int i;
   for(i = 0; i < indent; i++)
     if(!io_ch(value, " ", 1)) 
        return 0;
   return 1;
}

static int do_esc_char1(unsigned long c, unsigned char flags, char *do_quotes, char_io *io_ch, char *arg)
{
   unsigned char chflgs, chtmp;
   char tmphex[HEX_SIZE(long)+3]={0};

   if(c > 0xffffffffL)
      return -1;
   if(c > 0xffff) 
   {
      sprintf(tmphex, "%d\\W%08lX",sizeof(tmphex), c);
      if(!io_ch(arg, tmphex, 10)) 
         return -1;
      return 10;
   }
   if(c > 0xff) 
   {
      sprintf(tmphex, "%d\\U%04lX",sizeof(tmphex), c);
      if(!io_ch(arg, tmphex, 6)) 
         return -1;
      return 6;
   }
   chtmp = (unsigned char)c;
   if(chtmp > 0x7f) 
      chflgs = flags & ASN1_STRFLGS_ESC_MSB;
   else 
      chflgs = char_type[chtmp] & flags;
   if(chflgs & CHARTYPE_BS_ESC) 
   {
      /* If we don't escape with quotes, signal we need quotes */
      if(chflgs & ASN1_STRFLGS_ESC_QUOTE) 
      {
         if(do_quotes) 
            *do_quotes = 1;
         if(!io_ch(arg, &chtmp, 1)) 
            return -1;
         return 1;
      }
      if(!io_ch(arg, "\\", 1)) 
         return -1;
      if(!io_ch(arg, &chtmp, 1)) 
         return -1;
      return 2;
   }
   if(chflgs & (ASN1_STRFLGS_ESC_CTRL|ASN1_STRFLGS_ESC_MSB)) 
   {
      sprintf(tmphex,"%s\\%02X","11", chtmp);
      if(!io_ch(arg, tmphex, 3)) 
         return -1;
      return 3;
   }
   /* If we get this far and do any escaping at all must escape 
    * the escape character itself: backslash.
    */
   if (chtmp == '\\' && flags & ESC_FLAGS) 
   {
      if(!io_ch(arg, "\\\\", 2)) 
         return -1;
      return 2;
   }
   if(!io_ch(arg, &chtmp, 1)) 
      return -1;
   return 1;
}

static int do_buf1(unsigned char *buf, int buflen,
                        int type, unsigned char flags, char *quotes, char_io *io_ch, char *arg)
{
   int i, outlen, len;
   unsigned char orflags, *p, *q;
   unsigned long c;
   p = buf;
   q = buf + buflen;
   outlen = 0;
   while(p != q) 
   {
      if(p == buf && flags & ASN1_STRFLGS_ESC_2253) 
         orflags = CHARTYPE_FIRST_ESC_2253;
      else 
         orflags = 0;
      switch(type & BUF_TYPE_WIDTH_MASK) 
      {
         case 4:
            c = ((unsigned long)*p++) << 24;
            c |= ((unsigned long)*p++) << 16;
            c |= ((unsigned long)*p++) << 8;
            c |= *p++;
            break;
         case 2:
            c = ((unsigned long)*p++) << 8;
            c |= *p++;
            break;
         case 1:
            c = *p++;
            break;
         case 0:
            i = UTF8_getc(p, buflen, &c);
            if(i < 0) 
               return -1;    /* Invalid UTF8String */
            p += i;
            break;
         default:
            return -1;      /* invalid width */
      }
      if (p == q && flags & ASN1_STRFLGS_ESC_2253) 
         orflags = CHARTYPE_LAST_ESC_2253;
      if(type & BUF_TYPE_CONVUTF8) 
      {
         unsigned char utfbuf[6];
         int utflen;
         utflen = UTF8_putc(utfbuf, sizeof utfbuf, c);
         for(i = 0; i < utflen; i++) 
         {
            /* We don't need to worry about setting orflags correctly
             * because if utflen==1 its value will be correct anyway 
             * otherwise each character will be > 0x7f and so the 
             * character will never be escaped on first and last.
             */
             len = do_esc_char1(utfbuf[i], (unsigned char)(flags | orflags), quotes, io_ch, arg);
             if(len < 0) 
                return -1;
             outlen += len;
         }
      } 
      else 
      {
         len = do_esc_char1(c, (unsigned char)(flags | orflags), quotes, io_ch, arg);
         if(len < 0) 
            return -1;
         outlen += len;
     }
  }
  return outlen;
}

static int do_hex_dump1(char_io *io_ch, char *arg, unsigned char *buf, int buflen)
{
   static const char hexdig[] = "0123456789ABCDEF";
   unsigned char *p, *q;
   char hextmp[2]={0};
   if(arg) 
   {
      p = buf;
      q = buf + buflen;
      while(p != q) 
      {
         hextmp[0] = hexdig[*p >> 4];
         hextmp[1] = hexdig[*p & 0xf];
         if(!io_ch(arg, hextmp, 2)) 
            return -1;
         p++;
      }
   }
   return buflen << 1;
}

static int do_dump1(unsigned long lflags, char_io *io_ch, char *arg, ASN1_STRING *str)
{
   /* Placing the ASN1_STRING in a temp ASN1_TYPE allows
    * the DER encoding to readily obtained
    */
   ASN1_TYPE t;
   unsigned char *der_buf=NULL, *p=NULL;
   int outlen, der_len;

   if(!io_ch(arg, "#", 1)) 
      return -1;
   /* If we don't dump DER encoding just dump content octets */
   if(!(lflags & ASN1_STRFLGS_DUMP_DER)) 
   {
      outlen = do_hex_dump1(io_ch, arg, str->data, str->length);
      if(outlen < 0) 
         return -1;
      return outlen + 1;
   }
   t.type = str->type;
   t.value.ptr = (char *)str;
   der_len = i2d_ASN1_TYPE(&t, NULL);
   der_buf = OPENSSL_malloc(der_len);
   if(!der_buf) 
      return -1;
   p = der_buf;
   i2d_ASN1_TYPE(&t, &p);
   outlen = do_hex_dump1(io_ch, arg, der_buf, der_len);
   OPENSSL_free(der_buf);
   p=NULL;
   if(outlen < 0) 
      return -1;
   return outlen + 1;
}

static int do_ex(char_io *io_ch, char *arg, unsigned long lflags, ASN1_STRING *str)
{
   int outlen, len;
   int type;
   char quotes;
   unsigned char flags;

   quotes = 0;
   /* Keep a copy of escape flags */
   flags = (unsigned char)(lflags & ESC_FLAGS);
   type = str->type;
   outlen = 0;
   if(lflags & ASN1_STRFLGS_SHOW_TYPE) 
   {
      const char *tagname;
      tagname = ASN1_tag2str(type);
      outlen += strlen(tagname);
      if(!io_ch(arg, tagname, outlen) || !io_ch(arg, ":", 1)) 
         return -1;
      outlen++;
   }
   /* Decide what to do with type, either dump content or display it */

   /* Dump everything */
   if(lflags & ASN1_STRFLGS_DUMP_ALL) 
      type = -1;
   /* Ignore the string type */
   else if(lflags & ASN1_STRFLGS_IGNORE_TYPE) 
      type = 1;
   else 
   {
      /* Else determine width based on type */
         if((type > 0) && (type < 31)) 
            type = tag2nbyte[type];
         else 
            type = -1;
         if((type == -1) && !(lflags & ASN1_STRFLGS_DUMP_UNKNOWN)) 
            type = 1;
   }
   if(type == -1) 
   {
      len = do_dump1(lflags, io_ch, arg, str);
      if(len < 0) 
         return -1;
      outlen += len;
      return outlen;
   }
   if(lflags & ASN1_STRFLGS_UTF8_CONVERT) 
   {
      /* Note: if string is UTF8 and we want
       * to convert to UTF8 then we just interpret
       * it as 1 byte per character to avoid converting
       * twice.
       */
       if(!type) 
          type = 1;
       else 
          type |= BUF_TYPE_CONVUTF8;
   }
   len = do_buf1(str->data, str->length, type, flags, &quotes, io_ch, NULL);
   if(len < 0) 
      return -1;
   outlen += len;
   if(quotes) 
      outlen += 2;
   if(!arg) 
      return outlen;
   if(quotes && !io_ch(arg, "\"", 1)) 
      return -1;
   if(do_buf1(str->data, str->length, type, flags, NULL, io_ch, arg) < 0)
      return -1;
   if(quotes && !io_ch(arg, "\"", 1)) 
      return -1;
   return outlen;
}

static void get_name_ex(char_io *io_ch, char *value, X509_NAME *n,int indent,unsigned long flags)
{
   int i, prev = -1, orflags, cnt;
   int fn_opt, fn_nid;
   ASN1_OBJECT *fn;
   ASN1_STRING *val;
   X509_NAME_ENTRY *ent;
   char objtmp[80]={0};
   const char *objbuf;
   int len;
   char *sep_dn, *sep_mv, *sep_eq;
   int sep_dn_len, sep_mv_len, sep_eq_len;

   if(!do_indent(io_ch, value, indent)) 
     return ;
   switch (flags & XN_FLAG_SEP_MASK)
   {
      case XN_FLAG_SEP_MULTILINE:
         sep_dn = "\n";
         sep_dn_len = 1;
         sep_mv = " + ";
         sep_mv_len = 3;
         break;
      case XN_FLAG_SEP_COMMA_PLUS:
         sep_dn = ",";
         sep_dn_len = 1;
         sep_mv = "+";
         sep_mv_len = 1;
         indent = 0;
         break;
      case XN_FLAG_SEP_CPLUS_SPC:
         sep_dn = ", ";
         sep_dn_len = 2;
         sep_mv = " + ";
         sep_mv_len = 3;
         indent = 0;
         break;
      case XN_FLAG_SEP_SPLUS_SPC:
         sep_dn = "; ";
         sep_dn_len = 2;
         sep_mv = " + ";
         sep_mv_len = 3;
         indent = 0;
         break;
      default:
         return;
   }
   if(flags & XN_FLAG_SPC_EQ) 
   {
      sep_eq = " = ";
      sep_eq_len = 3;
   } 
   else 
   {
      sep_eq = "=";
      sep_eq_len = 1;
   }
   fn_opt = flags & XN_FLAG_FN_MASK;
   cnt = X509_NAME_entry_count(n);
   for(i = 0; i < cnt; i++) 
   {
      if(flags & XN_FLAG_DN_REV)
         ent = X509_NAME_get_entry(n, cnt - i - 1);
      else 
         ent = X509_NAME_get_entry(n, i);
      if(prev != -1) 
      {
         if(prev == ent->set) 
         {
            if(!io_ch(value, sep_mv, sep_mv_len)) 
               return ;
         } 
         else 
         {
            if(!io_ch(value, sep_dn, sep_dn_len)) 
               return ;
            if(!do_indent(io_ch, value, indent)) 
               return ;
         }
      }
      prev = ent->set;
      fn = X509_NAME_ENTRY_get_object(ent);
      val = X509_NAME_ENTRY_get_data(ent);
      fn_nid = OBJ_obj2nid(fn);
      if(fn_opt != XN_FLAG_FN_NONE) 
      {
         int objlen, fld_len;
         if((fn_opt == XN_FLAG_FN_OID) || (fn_nid==NID_undef) ) 
         {
            OBJ_obj2txt(objtmp, sizeof objtmp, fn, 1);
            fld_len = 0;
            objbuf = objtmp;
         } 
         else 
         {
            if(fn_opt == XN_FLAG_FN_SN) 
            {
               fld_len = FN_WIDTH_SN;
               objbuf = OBJ_nid2sn(fn_nid);
            } 
            else if(fn_opt == XN_FLAG_FN_LN) 
            {
               fld_len = FN_WIDTH_LN;
               objbuf = OBJ_nid2ln(fn_nid);
            } 
            else 
            {
               fld_len = 0; 
               objbuf = "";
            }
         }
         objlen = strlen(objbuf);
         if(!io_ch(value, objbuf, objlen)) 
            return ;
         if ((objlen < fld_len) && (flags & XN_FLAG_FN_ALIGN)) 
         {
            if (!do_indent(io_ch, value, fld_len - objlen)) 
               return ;
         }
         if(!io_ch(value, sep_eq, sep_eq_len)) 
            return ;
      }
      /* If the field name is unknown then fix up the DER dump
      * flag. We might want to limit this further so it will
      * DER dump on anything other than a few 'standard' fields.
      */
      if((fn_nid == NID_undef) && (flags & XN_FLAG_DUMP_UNKNOWN_FIELDS))
         orflags = ASN1_STRFLGS_DUMP_ALL;
      else 
         orflags = 0;
      len = do_ex(io_ch, value, flags | orflags, val);
      if(len < 0) 
         return ;
   }  
}

static void x509_name_get_ex(char *value, X509_NAME *nm,int indent, unsigned long flags)
{
   //if(flags == XN_FLAG_COMPAT)
     // X509_NAME_print(out, nm, indent);
    get_name_ex(send_bio_char, value, nm,indent ,flags);
}

static void get_gens(char *value, STACK_OF(GENERAL_NAME) *gens)
{
   int i,j;
   unsigned char *p=NULL;
   GENERAL_NAME *gen;
   char buf[80]={0};
   char *tmp=NULL;
   for (i = 0; i < sk_GENERAL_NAME_num(gens); i++)
   {
      gen = sk_GENERAL_NAME_value(gens, i);
      switch (gen->type)
      {
         case GEN_OTHERNAME:
            strcat(value, "  othername:<unsupported>");
            break;
         case GEN_X400:
            strcat(value, "   X400Name:<unsupported>");
            break;
         case GEN_EDIPARTY:
            strcat(value, "  EdiPartyName:<unsupported>");
            break;
         case GEN_EMAIL:
            strcat(value, "  email:");
            strcat(value, gen->d.ia5->data);
            break;
         case GEN_DNS:
            strcat(value, "  DNS:");
            strcat(value, gen->d.ia5->data);
            break;
         case GEN_URI:
            strcat(value, "  URI:");
            strcat(value, gen->d.ia5->data);
            break;
         case GEN_DIRNAME:
            strcat(value, "  DirName: ");
            x509_name_get_ex(value, gen->d.dirn,0,XN_FLAG_ONELINE);
            break;
         case GEN_IPADD:
            p = gen->d.ip->data;
            if(gen->d.ip->length == 4)
            {
               sprintf(buf,"  IP Address:%d.%d.%d.%d",p[0], p[1], p[2], p[3]);
               strcat(value, buf);
            }
            else if(gen->d.ip->length == 16)
            {
               strcat(value, "  IP Address");
               for (j = 0; j < 8; j++)
               {
                  sprintf(buf,":%X", p[0] << 8 | p[1]);    
                  strcat(value, buf);
                  p += 2;
               }
            }
            else
            {
               strcat(value, "  IP Address:<invalid>");  
               break;
            }
            break;
         case GEN_RID:
            strcat(value, "  Registered ID"); 
            j=i2t_ASN1_OBJECT(buf,sizeof buf,gen->d.rid);
            if (j > 0)
            {
               tmp = malloc(j + 1);
               i2t_ASN1_OBJECT(tmp,j + 1,gen->d.rid);
               strcat(value, tmp);
               free(tmp);
               tmp = NULL;
            }    
            break;
      }            
      strcat(value, "\n");
      p=NULL;
   }                
}

static void get_dispoint(char *value,DIST_POINT_NAME *dpn)
{
   X509_NAME ntmp;

   if (dpn->type == 0)
   {
      strcat(value,"Full Name:");
      strcat(value,"\n");
      get_gens(value,dpn->name.fullname);      
   }
   else
   {
      ntmp.entries = dpn->name.relativename;
      strcat(value,"Relative Name:");
      strcat(value,"\n");
      x509_name_get_ex(value, &ntmp,0, XN_FLAG_ONELINE);
      strcat(value,"\n");    
   }   
}

static void get_reasons( char *value, const char *rname,ASN1_BIT_STRING *rflags)
{
    int first = 1;
    const BIT_STRING_BITNAME *pbn;

    strcat(value,rname);
    for (pbn = reason_flags; pbn->lname; pbn++)
    {
       if (ASN1_BIT_STRING_get_bit(rflags, pbn->bitnum))
       {
          if (first)
             first = 0;
          else
             strcat(value,", ");
          strcat(value,pbn->lname);    
       }
    }   
    if (first)
       strcat(value,"<EMPTY>");
    strcat(value,"\n");
}

static void get_generalizedtime(char *value, const ASN1_GENERALIZEDTIME *tm)
{
   char *v=NULL;
   int gmt=0;
   int i;
   int y=0,M=0,d=0,h=0,m=0,s=0;
   char *f = NULL;
   int f_len = 0;
   char tmp[80]={0};

   i=tm->length;
   v=(char *)tm->data;

   if (i < 12)
   { 
      strcat(value,"Bad time value");
      return;
   }
   if (v[i-1] == 'Z') 
      gmt=1;
   for (i=0; i<12; i++)
      if ((v[i] > '9') || (v[i] < '0')) 
      {
         strcat(value,"Bad time value");
         return;   
      }
   y= (v[0]-'0')*1000+(v[1]-'0')*100 + (v[2]-'0')*10+(v[3]-'0');
   M= (v[4]-'0')*10+(v[5]-'0');
   if ((M > 12) || (M < 1)) 
   {
      strcat(value,"Bad time value");
      return; 
   }   
   d= (v[6]-'0')*10+(v[7]-'0');
   h= (v[8]-'0')*10+(v[9]-'0');
   m=  (v[10]-'0')*10+(v[11]-'0');
   if (tm->length >= 14 &&(v[12] >= '0') && (v[12] <= '9') &&(v[13] >= '0') && (v[13] <= '9'))
   {
      s=  (v[12]-'0')*10+(v[13]-'0');
      /* Check for fractions of seconds. */
      if (tm->length >= 15 && v[14] == '.')
      {
         int l = tm->length;
         f = &v[14];     /* The decimal point. */
         f_len = 1;
         while (14 + f_len < l && f[f_len] >= '0' && f[f_len] <= '9')
            ++f_len;
      }
   }
   sprintf(tmp,"%s %2d %02d:%02d:%02d%.*s %d%s",mon[M-1],d,h,m,s,f_len,f,y,(gmt)?" GMT":"");       
   strcat(value,tmp);
   memset(tmp,0,sizeof(tmp));
   v=NULL;
   f=NULL;   
}

static void get_pkey_usage_period( PKEY_USAGE_PERIOD *usage,char *value)
{
   if(usage->notBefore) 
   {
      strcat(value,"Not Before: ");     
      get_generalizedtime(value, usage->notBefore);
      if(usage->notAfter) 
         strcat(value,", ");
   }
   if(usage->notAfter) 
   {
      strcat(value,"Not After: ");
      get_generalizedtime(value, usage->notAfter);
   }
}

static void get_crldp( void *pcrldp,char *value)
{
   STACK_OF(DIST_POINT) *crld = pcrldp;
   DIST_POINT *point;
   int i;

   for(i = 0; i < sk_DIST_POINT_num(crld); i++)
   {
      point = sk_DIST_POINT_value(crld, i);
      if(point->distpoint)
         get_dispoint(value,point->distpoint);
      if(point->reasons)     
         get_reasons(value, "Reasons:", point->reasons);
      if(point->CRLissuer)
      {
         strcat(value,"CRL Issuer:");
         strcat(value,"\n");
         get_gens(value, point->CRLissuer);
      }
   }
}

static void get_certpol(STACK_OF(POLICYINFO) *ext_str,char *value)
{
   POLICYINFO *pinfo;
   POLICYQUALINFO *qualinfo;
   USERNOTICE *notice;
   ASN1_INTEGER *num;
   int i,j,n;
   char buf[80]={0},*tmp=NULL;
   
   for(i = 0; i < sk_POLICYINFO_num(ext_str); i++) 
   {
      pinfo = sk_POLICYINFO_value(ext_str, i);
      strcat(value, "Policy: ");
      j=i2t_ASN1_OBJECT(buf,sizeof buf,pinfo->policyid);
      if (j > 0)
      {
         tmp = malloc(j + 1);
         i2t_ASN1_OBJECT(tmp,j + 1,pinfo->policyid);
         strcat(value, tmp);
         free(tmp);
         tmp = NULL;
      }
      if(pinfo->qualifiers)
      {
         strcat(value,"\n");
         for(j = 0; j < sk_POLICYQUALINFO_num(pinfo->qualifiers); j++) 
         {
            qualinfo = sk_POLICYQUALINFO_value(pinfo->qualifiers, j);
            switch(OBJ_obj2nid(qualinfo->pqualid))
            {
               case NID_id_qt_cps:
                  strcat(value, "  CPS: ");
                  strcat(value,qualinfo->d.cpsuri->data);
                  strcat(value,"\n");
                  break;
               case NID_id_qt_unotice:
                  strcat(value, "  User Notice: ");
                  notice = qualinfo->d.usernotice;
                  if(notice->noticeref)
                  {
                     strcat(value,"\n");
                     strcat(value,"    Organization:");
                     strcat(value,notice->noticeref->organization->data);
                     strcat(value,"\n");
                     strcat(value,"    Number:");
                     for(n = 0; n < sk_ASN1_INTEGER_num(notice->noticeref->noticenos); n++) 
                     {
                        num = sk_ASN1_INTEGER_value(notice->noticeref->noticenos, n);
                        if(n) strcat(value,", ");
                        tmp = i2s_ASN1_INTEGER(NULL, num);
                        strcat(value,tmp);
                        tmp = NULL;                
                     }
                     strcat(value,"\n");
                  }
                  if(notice->exptext)
                  {
                     strcat(value,"\n");
                     strcat(value,"      Explicit Text:");
                     strcat(value,notice->exptext->data);
                     strcat(value,"\n");
                  }                    
                  break;
               default:
                  strcat(value, "  Unknown Qualifier: ");
                  n=i2t_ASN1_OBJECT(buf,sizeof buf,qualinfo->pqualid);
                  if (n > 0)
                  {
                     tmp = malloc(n + 1);
                     i2t_ASN1_OBJECT(tmp,n + 1,qualinfo->pqualid);
                     strcat(value, tmp);
                     free(tmp);
                     tmp = NULL;                        
                  }
                  break;
            }
         }
      }
   }
}

static void get_nc_ipadd(char *value, ASN1_OCTET_STRING *ip)
{
   int i, len;
   unsigned char *p;
   char tmp[80]={0};
   p = ip->data;
   len = ip->length;
   strcat(value,"IP:");
   if(len == 8)
   {
      sprintf(tmp,"%d.%d.%d.%d/%d.%d.%d.%d",p[0], p[1], p[2], p[3],p[4], p[5], p[6], p[7]);
      strcat(value,tmp);
   }
   else if(len == 32)
   {
      for (i = 0; i < 16; i++)
      {
         sprintf(tmp,"%X",p[0] << 8 | p[1]);
         strcat(value,tmp);
         p += 2;
         if (i == 7)
            strcat(value,"/");
         else if (i != 15)
            strcat(value,":");
      }
   }
   else
      strcat(value,"IP Address:<invalid>");
   memset(tmp,0,sizeof(tmp));
}

static void get_general_name(char *value, GENERAL_NAME *gen)
{
   unsigned char *p;
   int i,j;
   char tmp[80]={0};
   char *p1;

   switch (gen->type)
   {
      case GEN_OTHERNAME:
         strcat(value,"othername:<unsupported>");
         break;
      case GEN_X400:
         strcat(value,"X400Name:<unsupported>");
         break;
      case GEN_EDIPARTY:
         /* Maybe fix this: it is supported now */
         strcat(value,"EdiPartyName:<unsupported>");
         break;
      case GEN_EMAIL:
         sprintf(tmp,"email:%s",gen->d.ia5->data);
         strcat(value,tmp);
         break;
      case GEN_DNS:
         sprintf(tmp,"DNS:%s",gen->d.ia5->data);
         strcat(value,tmp); 
         break;
      case GEN_URI:
         sprintf(tmp,"URI:%s",gen->d.ia5->data);
         strcat(value,tmp); 
         break;
      case GEN_DIRNAME:
         strcat(value,"DirName: ");
         x509_name_get_ex(value, gen->d.dirn, 0, XN_FLAG_ONELINE);
         break;
      case GEN_IPADD:
         p = gen->d.ip->data;
         if(gen->d.ip->length == 4)
         {
            sprintf(tmp,"IP Address:%d.%d.%d.%d",p[0], p[1], p[2], p[3]);
            strcat(value,tmp);
         }
         else if(gen->d.ip->length == 16)
         {
            strcat(value,"IP Address");
            for (i = 0; i < 8; i++)
            {
               sprintf(tmp,":%X",p[0] << 8 | p[1]);
               strcat(value,tmp);
               p += 2;
            }
            strcat(value,"\n");
         }
         else
            strcat(value,"IP Address:<invalid>");
         break;
      case GEN_RID:
         strcat(value,"Registered ID");
         j=i2t_ASN1_OBJECT(tmp,sizeof tmp,gen->d.rid);
         if (j > 0)
         {
            p1 = malloc(j + 1);
            i2t_ASN1_OBJECT(p1,j + 1,gen->d.rid);
            strcat(value,p1);
            free(p1);
            p1 = NULL;
         }
         break;
   }
   memset(tmp,0,sizeof(tmp));
   p=NULL;
}

static void get_i2r_name_constraints( STACK_OF(GENERAL_SUBTREE) *trees,char *value,char *name)
{
   GENERAL_SUBTREE *tree;
   int i;

   if (sk_GENERAL_SUBTREE_num(trees) > 0)
   {
      strcat(value,name);
      strcat(value,"\n");
   }
   for(i = 0; i < sk_GENERAL_SUBTREE_num(trees); i++)
   {
      tree = sk_GENERAL_SUBTREE_value(trees, i);
      strcat(value,"  ");
      if (tree->base->type == GEN_IPADD)
         get_nc_ipadd(value, tree->base->d.ip);
      else
         get_general_name(value, tree->base);
      strcat(value,"\n");
   }

}

static void get_name_constraints(void *ext_str,char *value)
{
     NAME_CONSTRAINTS *ncons = ext_str;
     get_i2r_name_constraints(ncons->permittedSubtrees,value,"Permitted:");
     get_i2r_name_constraints(ncons->excludedSubtrees,value,"Excluded:");

}

//get expand value
static char* get_expand_value(X509_EXTENSION *ext, char *value,char *name)
{
   void *ext_str = NULL;
   const unsigned char *p;
   const X509V3_EXT_METHOD *method;
   STACK_OF(CONF_VALUE) *nval = NULL;
   CONF_VALUE *val;
   int i,j;

   strcpy(value,"\n");
   j=X509_EXTENSION_get_critical(ext);
   if(j != 0)
   {
      strcat(value,"critical");
      strcat(value,"\n");
   }  
   if(!(method = X509V3_EXT_get(ext)))
      return NULL;
   p = ext->value->data;
   if(method->it) 
      ext_str = ASN1_item_d2i(NULL, &p, ext->value->length, ASN1_ITEM_ptr(method->it));
   else 
      ext_str = method->d2i(NULL, &p, ext->value->length);
   if(!ext_str)
      return NULL;
   if(method->i2v)
   {
      nval = method->i2v(method, ext_str, NULL);
      for(i = 0; i < sk_CONF_VALUE_num(nval); i++) 
      {
         val = sk_CONF_VALUE_value(nval, i);
         if(!val->name)
         {
            if(i==0)
            {
               strcat(value,val->value);
               strcat(value,"\n");
            }
            else
            {               
               strcat(value,val->value);
               strcat(value,"\n");
            }
         }
         else if(!val->value)
         {
            if(i==0)
            {
               strcat(value,val->name);
               strcat(value,"\n");
            }
            else
            {
               strcat(value,val->name);
               strcat(value,"\n");
            }
         }      
         else 
         {
            strcat(value,val->name);
            strcat(value,":");
            strcat(value,val->value);
            strcat(value,"\n");
         }
      }
   }
   else if(method->i2s) 
      strcat(value,method->i2s(method, ext_str));
   else if(method->i2r) 
   {
      if(strcmp(name,"X509v3 CRL Distribution Points")==0)
         get_crldp(ext_str,value);
      else if (strcmp(name,"X509v3 Certificate Policies")==0)
         get_certpol(ext_str,value);
      else if (strcmp(name,"X509v3 Private Key Usage Period")==0)
         get_pkey_usage_period(ext_str,value); 
      else if (strcmp(name,"X509v3 Name Constraints")==0)
         get_name_constraints(ext_str,value);    
   }
   return value;
}
       
static void get_expand_info(STACK_OF(X509_EXTENSION) *exts)
{
   int i,j;
   char buf[80]={0};
   ASN1_OBJECT *obj;
   X509_EXTENSION *ex;
   char *name=NULL;
   char tmp_value[BUFSIZE]={0};
   char *expand_value=NULL;

   for (i=0; i<sk_X509_EXTENSION_num(exts); i++)
   {
      ex=sk_X509_EXTENSION_value(exts, i);
      obj=X509_EXTENSION_get_object(ex);
      j=i2t_ASN1_OBJECT(buf,sizeof buf,obj);
      if (j > 0)
      {
         name = malloc(j + 1);
         i2t_ASN1_OBJECT(name,j + 1,obj);
      }
      expand_value = get_expand_value(ex,tmp_value,name);
      if(!expand_value)
      {
         certificate.exinfo->oid=(char *)malloc(strlen(name)+1); 
         strcpy(certificate.exinfo->oid,name);
      }
      else if(strlen(expand_value))
      {
         for (j = 0; j < 15; j++) 
         {
            if (strcmp(name,exname[j])==0) 
            {
               switch (j) 
               {
                  case 0:
                     certificate.exinfo->basicConst=(char *)malloc(strlen(expand_value)+1); 
                     strcpy(certificate.exinfo->basicConst,expand_value);
                     break;
                  case 1:
                     certificate.exinfo->keyUsage=(char *)malloc(strlen(expand_value)+1); 
                     strcpy(certificate.exinfo->keyUsage,expand_value);
                     break;                 
                  case 2:
                     certificate.exinfo->certPolicies=(char *)malloc(strlen(expand_value)+1); 
                     strcpy(certificate.exinfo->certPolicies,expand_value);
                     break;
                  case 3:
                     certificate.exinfo->crlDist=(char *)malloc(strlen(expand_value)+1); 
                     strcpy(certificate.exinfo->crlDist,expand_value);
                     break;
                  case 4:
                     certificate.exinfo->exkeyUsage=(char *)malloc(strlen(expand_value)+1); 
                     strcpy(certificate.exinfo->exkeyUsage,expand_value);
                     break;
                  case 5:
                     certificate.exinfo->infoAccess=(char *)malloc(strlen(expand_value)+1); 
                     strcpy(certificate.exinfo->infoAccess,expand_value);
                     break;
                  case 6:
                     certificate.exinfo->autokeyIdentifier=(char *)malloc(strlen(expand_value)+1); 
                     strcpy(certificate.exinfo->autokeyIdentifier,expand_value);
                     break;
                  case 7:
                     certificate.exinfo->netscertType=(char *)malloc(strlen(expand_value)+1); 
                     strcpy(certificate.exinfo->netscertType,expand_value);
                     break;
                  case 8:
                     certificate.exinfo->subkeyIdentifier=(char *)malloc(strlen(expand_value)+1); 
                     strcpy(certificate.exinfo->subkeyIdentifier,expand_value);
                     break;
                  case 9:
                     certificate.exinfo->subaltName=(char *)malloc(strlen(expand_value)+1); 
                     strcpy(certificate.exinfo->subaltName,expand_value);
                     break;
                  case 10:
                     certificate.exinfo->issaltName=(char *)malloc(strlen(expand_value)+1); 
                     strcpy(certificate.exinfo->issaltName,expand_value);
                     break;
                  case 11:
                     certificate.exinfo->netsComment=(char *)malloc(strlen(expand_value)+1); 
                     strcpy(certificate.exinfo->netsComment,expand_value);
                     break;
                  case 12:
                     certificate.exinfo->prikeyPeriod=(char *)malloc(strlen(expand_value)+1); 
                     strcpy(certificate.exinfo->prikeyPeriod,expand_value);
                     break;
                  case 13:
                     certificate.exinfo->nameConstraints=(char *)malloc(strlen(expand_value)+1); 
                     strcpy(certificate.exinfo->nameConstraints,expand_value);
                     break;
                  case 14:
                     certificate.exinfo->polMappings=(char *)malloc(strlen(expand_value)+1); 
                     strcpy(certificate.exinfo->polMappings,expand_value);
                     break;
               }
            }
         }         
         memset(tmp_value,0,sizeof(tmp_value));
      }
      free(name);
      name = NULL;
   }   
}

static void free_single_certificate_memeory()
{
   free(certificate.by->commonName);
   free(certificate.by->countryName);
   free(certificate.by->localityName);
   free(certificate.by->provinceName);
   free(certificate.by->organizationName);
   free(certificate.by->organizationalUnitName);
   free(certificate.by->organizationalUnitName1);
   memset(certificate.by,0,sizeof(*(certificate.by)));

   free(certificate.to->commonName);
   free(certificate.to->countryName);
   free(certificate.to->localityName);
   free(certificate.to->provinceName);
   free(certificate.to->organizationName);
   free(certificate.to->organizationalUnitName);
   free(certificate.to->organizationalUnitName1);
   memset(certificate.to,0,sizeof(*(certificate.to)));

   free(certificate.exinfo->basicConst);
   free(certificate.exinfo->keyUsage);
   free(certificate.exinfo->certPolicies);
   free(certificate.exinfo->crlDist);
   free(certificate.exinfo->exkeyUsage);
   free(certificate.exinfo->infoAccess);
   free(certificate.exinfo->autokeyIdentifier);
   free(certificate.exinfo->netscertType);
   free(certificate.exinfo->subkeyIdentifier);
   free(certificate.exinfo->subaltName);
   free(certificate.exinfo->issaltName);
   free(certificate.exinfo->netsComment);
   free(certificate.exinfo->nameConstraints);
   free(certificate.exinfo->prikeyPeriod);
   free(certificate.exinfo->polMappings);
   free(certificate.exinfo->oid);
   memset(certificate.exinfo,0,sizeof(*(certificate.exinfo)));

   certificate.sig_alg=NULL;
   certificate.signature=NULL;
   certificate.pkey_alg=NULL;
   certificate.pkey_value=NULL;  
}

//get single certificate information
void get_single_certificate_data(const char *certificateData)
{
   BIO *bio_mem=NULL;
   X509 *x509=NULL;
   PKCS12 *pk12 = NULL; 
   STACK_OF(X509)* ca = NULL;

   int i,Nid,entriesNum;

   X509_NAME_ENTRY *name_entry;
   X509_NAME *issuer,*subject;
   ASN1_INTEGER *serialnum;
   char tmpdata[3];
   char date[TIMELENGTH]={0};
   unsigned int fingerNum;
   unsigned char md[EVP_MAX_MD_SIZE]={0};
   const EVP_MD *fdig=NULL;
   EVP_PKEY *pkey;

   if(certificate.sig_alg)
   {
      free_single_certificate_memeory();
      free(ce_chain_value);
      ce_chain_value=NULL;
      free(ce_chain_value1);
      ce_chain_value1=NULL;
      free(ce_chain_value2);
      ce_chain_value2=NULL;
   }
   //p12单独处理
   if(certificate_password)
   {
      if(ce_chain_value)
                {
         free(ce_chain_value);
         ce_chain_value=NULL;
                }
      SSLeay_add_all_algorithms();  
      ERR_load_crypto_strings();
      bio_mem=BIO_new_file(certificateData,"r");
      export_personal_path=(char *)malloc(strlen(certificateData)+1);
      strcpy(export_personal_path,certificateData);
      pk12 = d2i_PKCS12_bio(bio_mem, NULL);
      BIO_free_all(bio_mem);
      PKCS12_parse (pk12, certificate_password, &pkey, &x509, &ca);
      PKCS12_free(pk12);
      certificate_password = NULL;
   } 
   else
   {
      if(!ce_chain_value)
      {
         ce_chain_value = (char *)malloc(strlen(certificateData)+1);
         strcpy(ce_chain_value,certificateData);
      }
      bio_mem = BIO_new(BIO_s_mem());
      BIO_puts(bio_mem, certificateData);
      x509 = PEM_read_bio_X509(bio_mem, NULL, NULL, NULL);
      BIO_free(bio_mem);
   }
   memset(&certificate,0,sizeof(struct Certificate));
  
   certificate.by = (struct Issued_By *)malloc(sizeof(struct Issued_By));
   memset(certificate.by,0,sizeof(struct Issued_By));

   certificate.to = (struct Issued_To *)malloc(sizeof(struct Issued_To));
   memset(certificate.to,0,sizeof(struct Issued_To));
  
   //version
   sprintf(certificate.version,"%ld",X509_get_version(x509)+1);  

  //issure
   issuer = X509_get_issuer_name(x509);
   entriesNum = sk_X509_NAME_ENTRY_num(issuer->entries);
   for(i=0;i<entriesNum;i++)
   {
       name_entry = sk_X509_NAME_ENTRY_value(issuer->entries,i);
       Nid = OBJ_obj2nid(name_entry->object);
       switch(Nid)
       {
          case NID_countryName:
            certificate.by->countryName=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(certificate.by->countryName,name_entry->value->data);
            break;
          case NID_stateOrProvinceName: 
            certificate.by->provinceName=(char *)malloc(strlen(name_entry->value->data)+1); 
            strcpy(certificate.by->provinceName,name_entry->value->data);
            break;
          case NID_localityName:
            certificate.by->localityName=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(certificate.by->localityName,name_entry->value->data);
            break;
          case NID_organizationName:
            certificate.by->organizationName=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(certificate.by->organizationName,name_entry->value->data);
            break;
          case NID_organizationalUnitName:
            if(certificate.by->organizationalUnitName==NULL)
            {
               certificate.by->organizationalUnitName=(char *)malloc(strlen(name_entry->value->data)+1);
               strcpy(certificate.by->organizationalUnitName,name_entry->value->data);
            }
            else
            {
               certificate.by->organizationalUnitName1=(char *)malloc(strlen(name_entry->value->data)+1);
               strcpy(certificate.by->organizationalUnitName1,name_entry->value->data);
            }
            break;
          case NID_commonName:
            certificate.by->commonName=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(certificate.by->commonName,name_entry->value->data);
            break;
      }
   }
  
  //subject
   subject = X509_get_subject_name(x509);
   entriesNum = sk_X509_NAME_ENTRY_num(subject->entries);
   for(i=0;i<entriesNum;i++)
   {
       name_entry = sk_X509_NAME_ENTRY_value(subject->entries,i);
       Nid = OBJ_obj2nid(name_entry->object);
       switch(Nid)
       {
          case NID_countryName:
            certificate.to->countryName=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(certificate.to->countryName,name_entry->value->data);
            break;
          case NID_stateOrProvinceName: 
            certificate.to->provinceName=(char *)malloc(strlen(name_entry->value->data)+1); 
            strcpy(certificate.to->provinceName,name_entry->value->data);
            break;
          case NID_localityName:
            certificate.to->localityName=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(certificate.to->localityName,name_entry->value->data);
            break;
          case NID_organizationName:
            certificate.to->organizationName=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(certificate.to->organizationName,name_entry->value->data);
            break;
          case NID_organizationalUnitName:
            if(certificate.to->organizationalUnitName==NULL)
            {
               certificate.to->organizationalUnitName=(char *)malloc(strlen(name_entry->value->data)+1);
               strcpy(certificate.to->organizationalUnitName,name_entry->value->data);
            }
            else
            {
               certificate.to->organizationalUnitName1=(char *)malloc(strlen(name_entry->value->data)+1);
               strcpy(certificate.to->organizationalUnitName1,name_entry->value->data);
            }
            break;
          case NID_commonName:
            certificate.to->commonName=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(certificate.to->commonName,name_entry->value->data);
            break;
       }
    }

    //serial number 
    serialnum = X509_get_serialNumber(x509);
    for(i=0; i<serialnum->length; i++) 
    {
        sprintf(tmpdata,"%02x",serialnum->data[i]);
        
        if(i==0)
        {
           strcpy(certificate.serialNumber,tmpdata);
        }
        else
        {
           strcat(certificate.serialNumber,":");
           strcat(certificate.serialNumber,tmpdata);
        }
    }
 
    //period of validity
    strcpy(certificate.startDate,asn1_gettime(X509_get_notBefore(x509),date));
    strcpy(certificate.endDate,asn1_gettime(X509_get_notAfter(x509),date));

    //finger:sha1 information
    fdig = EVP_sha1();
    X509_digest(x509,fdig,md,&fingerNum);
    for (i=0; i<(int)fingerNum; i++)
    { 
        sprintf(tmpdata,"%02x",md[i]);
        if(i==0)
        {
           strcpy(certificate.sha1_t,tmpdata);
        }
        else
        {
           strcat(certificate.sha1_t,":");
           strcat(certificate.sha1_t,tmpdata);
        }          
    }

    //finger:md5 information
    fdig = EVP_md5();
    X509_digest(x509,fdig,md,&fingerNum);
    for (i=0; i<(int)fingerNum; i++)
    { 
        sprintf(tmpdata,"%02x",md[i]);
        if(i==0)
        {
           strcpy(certificate.md5_t,tmpdata);
        }
        else
        {
           strcat(certificate.md5_t,":");
           strcat(certificate.md5_t,tmpdata);
        }          
    }
    
    //Signature
    x509_algor(x509->sig_alg,&certificate.sig_alg);
    x509_asn1_string(x509->signature,&certificate.signature);

   //public key
   x509_algor(x509->cert_info->key->algor,&certificate.pkey_alg);   
   pkey=X509_get_pubkey(x509);
   if(strcmp(certificate.pkey_alg,"rsaEncryption")==0)
      x509_get_pkey_value(pkey->pkey.rsa,&certificate.pkey_value);
   
   //expand info   
   certificate.exinfo = (struct Expand *)malloc(sizeof(struct Expand));
   memset(certificate.exinfo,0,sizeof(*(certificate.exinfo)));
   get_expand_info(x509->cert_info->extensions); 
 
   X509_free(x509);  
}

static void check_certificate_verfity(char **value)
{
   BIO *bio_mem=NULL;
   X509_STORE_CTX *ctx=NULL;
   X509_STORE *certchain=NULL;   
   X509 *cert=NULL;
   X509 *rootcert=NULL;
   int x509verify;
   long ncode;
   //p12证书处理
   if(!ce_chain_value)
        {
      *value = NULL;
      return;
        }
   bio_mem=BIO_new(BIO_s_mem());
   BIO_puts(bio_mem,ce_chain_value);
   cert=PEM_read_bio_X509(bio_mem, NULL, NULL, NULL);
   BIO_free(bio_mem);
   certchain = X509_STORE_new();
   if(ce_chain_value1)
   {
      bio_mem=BIO_new(BIO_s_mem());
      BIO_puts(bio_mem,ce_chain_value1);
      rootcert=PEM_read_bio_X509(bio_mem, NULL, NULL, NULL);    
      BIO_free(bio_mem);
      X509_STORE_add_cert(certchain,rootcert);      
      X509_free(rootcert);
   }
   else
      X509_STORE_add_cert(certchain,cert);
   if(ce_chain_value2)
   {
      bio_mem=BIO_new(BIO_s_mem());
      BIO_puts(bio_mem,ce_chain_value2);
      rootcert=PEM_read_bio_X509(bio_mem, NULL, NULL, NULL);
      BIO_free(bio_mem);     
      X509_STORE_add_cert(certchain,rootcert);
      X509_free(rootcert);
   }
            
   ctx=X509_STORE_CTX_new();  
   X509_STORE_CTX_init(ctx,certchain,cert,NULL);
   x509verify=X509_verify_cert(ctx);
   if(x509verify!=1)
   {
      ncode=X509_STORE_CTX_get_error(ctx);
     *value=(char *)X509_verify_cert_error_string(ncode);
      if(strcmp(*value,"certificate signature failure")==0)
         *value=NULL;
   }
   if(ctx!=NULL)
      X509_STORE_CTX_free(ctx);
   if(certchain!=NULL)
      X509_STORE_free(certchain);
   X509_free(cert); 
}

static void get_root_path(char *root_path,char **toname,char **byname,char **path)
{
   DIR *dir=NULL;
   BIO *b=NULL;
   X509 *x=NULL;
   X509_NAME *issuer,*subject;
   int entriesNum,Nid;
   X509_NAME_ENTRY *name_entry;
   struct dirent *ptr=NULL;
   FILE *fp;
   int file_size;
   char pem[DER_MAX_SIZE]={0};
   char *p=NULL;
   int j;
   
   if((dir=opendir(root_path))==NULL)
      return;
   while((ptr=readdir(dir))!=NULL)
   {
      if((strcmp(ptr->d_name,".")==0) || (strcmp(ptr->d_name,"..")==0))
         continue;
      *path=(char *)malloc(strlen(ptr->d_name)+strlen(root_path)+1);
      strcpy(*path,root_path);
      strcat(*path,ptr->d_name);
      b=BIO_new_file(*path,"r");   
      x=PEM_read_bio_X509(b,NULL,NULL,NULL);
      BIO_free(b);
      if(!x)
      {
         fp=fopen(*path,"r");
         fseek(fp,0,SEEK_END);   
         file_size=ftell(fp);
         fseek(fp,0,SEEK_SET);
         fread(pem,file_size,1,fp);
         fclose(fp);
         p=pem;
         x=d2i_X509(NULL,(const unsigned char ** )&p,file_size);
         if(!x)
         { 
            //only support der or pem
            free(*path);
            *path=NULL;
            memset(pem,0,sizeof(pem));
            continue;
         }
         memset(pem,0,sizeof(pem));
      }
      subject=X509_get_subject_name(x);
      entriesNum=sk_X509_NAME_ENTRY_num(subject->entries);
      for(j=0;j<entriesNum;j++)
      {
         name_entry=sk_X509_NAME_ENTRY_value(subject->entries,j);
         Nid=OBJ_obj2nid(name_entry->object);
         if(Nid==NID_commonName)
         { 
            *toname=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(*toname,name_entry->value->data);    
            break;
         }
      }        
      if(*toname==NULL)
      { 
         X509_free(x);
         free(*path);
         *path=NULL;
         continue;
      }
      if(strcmp(*byname,*toname)==0)
      {
         issuer=X509_get_issuer_name(x); 
         entriesNum=sk_X509_NAME_ENTRY_num(issuer->entries);
         for(j=0;j<entriesNum;j++)
         {
            name_entry=sk_X509_NAME_ENTRY_value(issuer->entries,j);
            Nid=OBJ_obj2nid(name_entry->object);
            if(Nid==NID_commonName)
            {
               free(*byname);
               *byname=NULL;
               *byname=(char *)malloc(strlen(name_entry->value->data)+1);
               strcpy(*byname,name_entry->value->data);
               break;
            }
         }
         X509_free(x);
         break;
      }
      else
      {
         X509_free(x);
         free(*toname);
         *toname=NULL;
         free(*path);
         *path=NULL;
      }
   }
}            

static void get_certificate_chain(char **value)
{
   int i,j;
   char *toname=NULL;
   char *byname=NULL;
   char *path=NULL;
   BIO *b=NULL;
   BIO *out=NULL;
   X509 *x=NULL;
   FILE *fp;
   int file_size;
   char pem[DER_MAX_SIZE]={0};
   char *p=NULL;
   char root_cerpath[CERTITLELENGTH]={0};

   if(!certificate.to->commonName)
   {
      if(certificate.to->organizationalUnitName)
      {
         value[0]=(char *)malloc(strlen(certificate.to->organizationalUnitName)+1);
         strcpy(value[0],certificate.to->organizationalUnitName);
      }
      else
      {
         value[0]=(char *)malloc(strlen(certificate.to->organizationName)+1);
         strcpy(value[0],certificate.to->organizationName);    
      }
      return;
   }
   else
   {
      toname=(char *)malloc(strlen(certificate.to->commonName)+1);
      strcpy(toname,certificate.to->commonName);
      byname=(char *)malloc(strlen(certificate.by->commonName)+1);
      strcpy(byname,certificate.by->commonName);
      for(i=0;i<CHAIN_MAX_SIZE;i++)
      {
         value[i]=(char *)malloc(strlen(toname) +1);
         strcpy(value[i],toname);
         free(toname);
         toname=NULL;
         if(strcmp(value[i],byname)==0)
         {
            free(byname);
            byname=NULL;
            break;
         }
         strcpy(root_cerpath,midori_paths_get_res_dir());
         strcat(root_cerpath,"/root_certificate/");
         get_root_path(root_cerpath,&toname,&byname,&path);
         if(!path)
         {
            strcpy(root_cerpath,getenv("HOME"));
            strcat(root_cerpath,"/.cache/webkitgtk/root_certificate/");
            get_root_path(root_cerpath,&toname,&byname,&path);
         }
         if(path)
         {
            //store certificate information
            b=BIO_new_file(path,"r");   
            x=PEM_read_bio_X509(b,NULL,NULL,NULL);
            if(x)
            {
               fp=fopen(path,"r");
               fseek(fp,0,SEEK_END);   
               file_size=ftell(fp);
               fseek(fp,0,SEEK_SET);
               fread(pem,file_size,1,fp);
               fclose(fp);
               p=pem;
               for(j=0;;p++,j++)
               {
                  if(*p=='E'&&strncmp(p,"END CERTIFICATE",strlen("END CERTIFICATE"))==0)
                  break;
               }
               if(!ce_chain_value1)
               {
                  ce_chain_value1=(char *)malloc(j+1+strlen("END CERTIFICATE-----"));
                  memcpy(ce_chain_value1,pem,j+strlen("END CERTIFICATE-----"));
                  ce_chain_value1[j+strlen("END CERTIFICATE-----")]='\0'; 
                  memset(pem,0,sizeof(pem)); 
               }
               else
               {
                  ce_chain_value2=(char *)malloc(j+1+strlen("END CERTIFICATE-----"));
                  memcpy(ce_chain_value2,pem,j+strlen("END CERTIFICATE-----"));
                  ce_chain_value2[j+strlen("END CERTIFICATE-----")]='\0';
                  memset(pem,0,sizeof(pem));         
               }
            }
            else
            {
               b=BIO_new_file(path,"r");
               x=d2i_X509_bio(b,NULL);
               out=BIO_new(BIO_s_mem());
               PEM_write_bio_X509(out,x);
               BIO_read(out,pem,DER_MAX_SIZE);
               BIO_free(out);
               p=pem;
               for(j=0;;p++,j++)
               {
                  if(*p=='E'&&strncmp(p,"END CERTIFICATE",strlen("END CERTIFICATE"))==0)
                  break;
               }
               if(!ce_chain_value1)
               {
                  ce_chain_value1=(char *)malloc(j+1+strlen("END CERTIFICATE-----"));
                  memcpy(ce_chain_value1,pem,j+strlen("END CERTIFICATE-----"));
                  ce_chain_value1[j+strlen("END CERTIFICATE-----")]='\0'; 
                  memset(pem,0,sizeof(pem));
               }
               else
               {
                  ce_chain_value2=(char *)malloc(j+1+strlen("END CERTIFICATE-----"));
                  memcpy(ce_chain_value2,pem,j+strlen("END CERTIFICATE-----"));
                  ce_chain_value2[j+strlen("END CERTIFICATE-----")]='\0';
                  memset(pem,0,sizeof(pem));                  
               }
            }
            BIO_free(b);
            X509_free(x);
            free(path);
            path=NULL;
         }
         if(!toname)
         {
            if(byname)
            {
               free(byname);
               byname=NULL;
               break;
            }
         }
      }
   }
}

static void get_tree_store(GtkTreeStore *tree_store)
{
   GtkTreeIter  iters;
   GtkTreeIter  iters_child,iters_child1,iters_child2;
   char tmp[80]={0};

   gtk_tree_store_clear(tree_store);
   gtk_tree_store_append(tree_store, &iters, NULL);
   if(certificate.to->commonName)
      gtk_tree_store_set(tree_store, &iters,0,certificate.to->commonName,-1);
   else if(certificate.to->organizationalUnitName)
      gtk_tree_store_set(tree_store, &iters ,0,certificate.to->organizationalUnitName,-1);
   else
      gtk_tree_store_set(tree_store, &iters ,0,certificate.to->organizationName,-1); 
   gtk_tree_store_append(tree_store, &iters_child, &iters);
   gtk_tree_store_set(tree_store, &iters_child ,0,"证书",-1);   
   gtk_tree_store_append(tree_store, &iters_child1, &iters_child);
   gtk_tree_store_set(tree_store, &iters_child1 ,0,"版本",-1);
   gtk_tree_store_append(tree_store, &iters_child1, &iters_child);
   gtk_tree_store_set(tree_store, &iters_child1 ,0,"序列号",-1);  
   gtk_tree_store_append(tree_store, &iters_child1, &iters_child);
   gtk_tree_store_set(tree_store, &iters_child1 ,0,"颁发者",-1);   
   gtk_tree_store_append(tree_store, &iters_child1, &iters_child);
   gtk_tree_store_set(tree_store, &iters_child1 ,0,"有效日期",-1);
   gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
   gtk_tree_store_set(tree_store, &iters_child2 ,0,"不早于",-1);
   gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
   gtk_tree_store_set(tree_store, &iters_child2 ,0,"不晚于",-1);
   gtk_tree_store_append(tree_store, &iters_child1, &iters_child);
   gtk_tree_store_set(tree_store, &iters_child1 ,0,"主题",-1);
   gtk_tree_store_append(tree_store, &iters_child1, &iters_child);
   gtk_tree_store_set(tree_store, &iters_child1 ,0,"公共密钥信息",-1);
   gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
   gtk_tree_store_set(tree_store, &iters_child2 ,0,"公共密钥算法",-1);
   gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
   gtk_tree_store_set(tree_store, &iters_child2 ,0,"公共密钥值",-1);
   gtk_tree_store_append(tree_store, &iters_child1, &iters_child);
   gtk_tree_store_set(tree_store, &iters_child1 ,0,"扩展",-1);
   if(certificate.exinfo->basicConst)
   {
      gtk_tree_store_append(tree_store,&iters_child2,&iters_child1);
      gtk_tree_store_set(tree_store, &iters_child2 ,0,"证书基本约束",-1);
   }
   if(certificate.exinfo->keyUsage)
   {
      gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
      gtk_tree_store_set(tree_store, &iters_child2 ,0,"证书密钥使用",-1);
   }
   if(certificate.exinfo->certPolicies)
   {
      gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
      gtk_tree_store_set(tree_store, &iters_child2 ,0,"证书方针",-1);
   }
   if(certificate.exinfo->crlDist)
   {
      gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
      gtk_tree_store_set(tree_store, &iters_child2 ,0,"CRL分布点",-1);
   }
   if(certificate.exinfo->exkeyUsage)
   {
      gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
      gtk_tree_store_set(tree_store, &iters_child2 ,0,"证书扩展密钥使用",-1);
   }
   if(certificate.exinfo->infoAccess)
   {
      gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
      gtk_tree_store_set(tree_store, &iters_child2 ,0,"授权信息访问",-1);
   }
   if(certificate.exinfo->autokeyIdentifier)
   {
      gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
      gtk_tree_store_set(tree_store, &iters_child2 ,0,"证书授权密钥身份",-1);
   }
   if(certificate.exinfo->netscertType)
   {
      gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
      gtk_tree_store_set(tree_store, &iters_child2 ,0,"Netscape证书类型",-1);
   }
   if(certificate.exinfo->subkeyIdentifier)
   {
      gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
      gtk_tree_store_set(tree_store, &iters_child2 ,0,"证书标题密钥身份",-1);
   }
   if(certificate.exinfo->subaltName)
   {
      gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
      gtk_tree_store_set(tree_store, &iters_child2 ,0,"证书主题可选名",-1);
   }
   if(certificate.exinfo->issaltName)
   {
      gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
      gtk_tree_store_set(tree_store, &iters_child2 ,0,"证书颁发可选名",-1);
   }
   if(certificate.exinfo->netsComment)
   {
      gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
      gtk_tree_store_set(tree_store, &iters_child2 ,0,"Netscape证书说明",-1);
   }
   if(certificate.exinfo->prikeyPeriod)
   {
      gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
      gtk_tree_store_set(tree_store, &iters_child2 ,0,"私有密钥有效期",-1);
   }
   if(certificate.exinfo->nameConstraints)
   {
      gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
      gtk_tree_store_set(tree_store, &iters_child2 ,0,"证书名称约束",-1);
   }
   if(certificate.exinfo->polMappings)
   {
      gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
      gtk_tree_store_set(tree_store, &iters_child2 ,0,"证书方针映射",-1);
   }
   if(certificate.exinfo->oid)
   {
      sprintf(tmp,"标识符%s",certificate.exinfo->oid);
      gtk_tree_store_append(tree_store, &iters_child2, &iters_child1);
      gtk_tree_store_set(tree_store, &iters_child2 ,0,tmp,-1);
   }
   gtk_tree_store_append(tree_store, &iters_child1, &iters_child);
   gtk_tree_store_set(tree_store, &iters_child1 ,0,"证书签名算法",-1);
   gtk_tree_store_append(tree_store, &iters_child1, &iters_child);
   gtk_tree_store_set(tree_store, &iters_child1 ,0,"证书签名值",-1);
   gtk_tree_view_expand_all((GtkTreeView *)tree2);
   memset(tmp,0,sizeof(tmp));
}

//获取证书链中（ce_chain_value，ce_chain_value1，ce_chain_value2）中主题名称
static void get_subject_name(char *value,char **name)
{
   X509 * x=NULL;
   BIO *b=NULL;
   X509_NAME *subject;
   int entriesNum,Nid,i;
   X509_NAME_ENTRY *name_entry;

   b=BIO_new(BIO_s_mem());
   BIO_puts(b,value);
   x=PEM_read_bio_X509(b, NULL, NULL, NULL);
   BIO_free(b);         
   subject=X509_get_subject_name(x);
   entriesNum=sk_X509_NAME_ENTRY_num(subject->entries);
   for(i=0;i<entriesNum;i++)
   {
      name_entry=sk_X509_NAME_ENTRY_value(subject->entries,i);
      Nid=OBJ_obj2nid(name_entry->object);
      if(Nid==NID_commonName)
      { 
         *name=(char *)malloc(strlen(name_entry->value->data)+1);
         strcpy(*name,name_entry->value->data);    
         break;
      }
   }
   X509_free(x);
}

static void on_changed_segment(GtkWidget *widget, GtkTreeStore *tree_store) 
{
   GtkTreeIter iter;
   GtkTreeModel *model;
   char *value; 
   char *name=NULL;

   if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) 
   {
      gtk_tree_model_get(model, &iter, 0, &value,  -1);
      if((certificate.to->commonName) && (strcmp(value,certificate.to->commonName)!=0))
      {
         get_subject_name(ce_chain_value,&name);     
         if((name) && (strcmp(value,name)==0))
         {
            free_single_certificate_memeory();
            get_single_certificate_data(ce_chain_value);
            get_tree_store(tree_store);
         }
         else
         {  
            if(name)
            {
               free(name);
               name=NULL;
            } 
            get_subject_name(ce_chain_value1,&name); 
            if((name) && (strcmp(value,name)==0))
            {
               free_single_certificate_memeory();
               get_single_certificate_data(ce_chain_value1);
               get_tree_store(tree_store);
            }
            else
            {  
               if(name)
               {
                  free(name);
                  name=NULL;
               }  
               get_subject_name(ce_chain_value2,&name);        
               if((name) && (strcmp(value,name)==0))
               {
                  free_single_certificate_memeory();
                  get_single_certificate_data(ce_chain_value2);
                  get_tree_store(tree_store);
               }                  
            }
         }
         free(name);
         name=NULL;
      }
      value=NULL;   
   }
}

static void on_changed_segment_value(GtkWidget *widget, GtkTreeStore *tree_store) 
{
   GtkTreeIter iter;
   GtkTreeModel *model;
   char *value; 
   char tmp_info[BUFSIZE]={0};


   if (gtk_tree_selection_get_selected(
      GTK_TREE_SELECTION(widget), &model, &iter)) 
   {
      gtk_tree_model_get(model, &iter, 0, &value,  -1);
      if(strcmp(value,"版本")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.version,-1);
      else if(strcmp(value,"序列号")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.serialNumber,-1);
      else if(strcmp(value,"颁发者")==0)
      {
         strcpy(tmp_info,"\n");
         if(certificate.by->commonName)
         {
            strcat(tmp_info,"CN = ");
            strcat(tmp_info,certificate.by->commonName);
            strcat(tmp_info,"\n");
         }  
         if(certificate.by->countryName && g_locale_to_utf8(certificate.by->countryName,-1,0,0,0))
         {
            strcat(tmp_info,"C = ");
            strcat(tmp_info,certificate.by->countryName);
            strcat(tmp_info,"\n");
         }
         if(certificate.by->localityName && g_locale_to_utf8(certificate.by->localityName,-1,0,0,0))
         {
            strcat(tmp_info,"L = ");
            strcat(tmp_info,certificate.by->localityName);
            strcat(tmp_info,"\n");
         }   
         if(certificate.by->provinceName && g_locale_to_utf8(certificate.by->provinceName,-1,0,0,0))
         {
            strcat(tmp_info,"ST = ");
            strcat(tmp_info,certificate.by->provinceName);
            strcat(tmp_info,"\n");
         }
         if(certificate.by->organizationName && g_locale_to_utf8(certificate.by->organizationName,-1,0,0,0))
         {
            strcat(tmp_info,"O = ");
            strcat(tmp_info,certificate.by->organizationName);
            strcat(tmp_info,"\n");
         }
         if(certificate.by->organizationalUnitName && g_locale_to_utf8(certificate.by->organizationalUnitName,-1,0,0,0))
         {
            strcat(tmp_info,"OU = ");
            strcat(tmp_info,certificate.by->organizationalUnitName);
            strcat(tmp_info,"\n");
         }
         if(certificate.by->organizationalUnitName1 && g_locale_to_utf8(certificate.by->organizationalUnitName1,-1,0,0,0))
         {
            strcat(tmp_info,"OU = ");
            strcat(tmp_info,certificate.by->organizationalUnitName1);
            strcat(tmp_info,"\n");
         }
         gtk_tree_store_set (tree_store, &iter_v ,0,tmp_info,-1);
         tmp_info[0]='\0';      
      }
      else if(strcmp(value,"不早于")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.startDate,-1);
      else if(strcmp(value,"不晚于")==0)
        gtk_tree_store_set (tree_store, &iter_v ,0,certificate.endDate,-1);
      else if(strcmp(value,"主题")==0)
      {
         strcpy(tmp_info,"\n");
         if(certificate.to->commonName)
         {
            strcat(tmp_info,"CN = ");
            strcat(tmp_info,certificate.to->commonName);
            strcat(tmp_info,"\n");
         }  
         if(certificate.to->countryName && g_locale_to_utf8(certificate.to->countryName,-1,0,0,0))
         {
            strcat(tmp_info,"C = ");
            strcat(tmp_info,certificate.to->countryName);
            strcat(tmp_info,"\n");
         }
         if(certificate.to->localityName && g_locale_to_utf8(certificate.to->localityName,-1,0,0,0))
         {
            strcat(tmp_info,"L = ");
            strcat(tmp_info,certificate.to->localityName);
            strcat(tmp_info,"\n");
         }   
         if(certificate.to->provinceName && g_locale_to_utf8(certificate.to->provinceName,-1,0,0,0))
         {
            strcat(tmp_info,"ST = ");
            strcat(tmp_info,certificate.to->provinceName);
            strcat(tmp_info,"\n");
         }
         if(certificate.to->organizationName && g_locale_to_utf8(certificate.to->organizationName,-1,0,0,0))
         {
            strcat(tmp_info,"O = ");
            strcat(tmp_info,certificate.to->organizationName);
            strcat(tmp_info,"\n");
         }
         if(certificate.to->organizationalUnitName && g_locale_to_utf8(certificate.to->organizationalUnitName,-1,0,0,0))
         {
            strcat(tmp_info,"OU = ");
            strcat(tmp_info,certificate.to->organizationalUnitName);
            strcat(tmp_info,"\n");
         }
         if(certificate.to->organizationalUnitName1 && g_locale_to_utf8(certificate.to->organizationalUnitName1,-1,0,0,0))
         {
            strcat(tmp_info,"OU = ");
            strcat(tmp_info,certificate.to->organizationalUnitName1);
            strcat(tmp_info,"\n");
         }
         gtk_tree_store_set (tree_store, &iter_v ,0,tmp_info,-1);
         memset(tmp_info,0,sizeof(tmp_info));      
      }
      else if(strcmp(value,"证书签名算法")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.sig_alg,-1);
      else if(strcmp(value,"证书签名值")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.signature,-1);
      else if(strcmp(value,"公共密钥算法")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.pkey_alg,-1);
      else if(strcmp(value,"公共密钥值")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.pkey_value,-1);
      else if(strcmp(value,"证书基本约束")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.exinfo->basicConst,-1);
      else if(strcmp(value,"证书密钥使用")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.exinfo->keyUsage,-1);
      else if(strcmp(value,"证书方针")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.exinfo->certPolicies,-1);
      else if(strcmp(value,"CRL分布点")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.exinfo->crlDist,-1);
      else if(strcmp(value,"证书扩展密钥使用")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.exinfo->exkeyUsage,-1);
      else if(strcmp(value,"授权信息访问")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.exinfo->infoAccess,-1);
      else if(strcmp(value,"证书授权密钥身份")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.exinfo->autokeyIdentifier,-1);
      else if(strcmp(value,"Netscape证书类型")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.exinfo->netscertType,-1);
      else if(strcmp(value,"证书标题密钥身份")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.exinfo->subkeyIdentifier,-1);
      else if(strcmp(value,"证书主题可选名")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.exinfo->subaltName,-1);
      else if(strcmp(value,"证书颁发可选名")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.exinfo->issaltName,-1);    
      else if(strcmp(value,"Netscape证书说明")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.exinfo->netsComment,-1); 
      else if(strcmp(value,"私有密钥有效期")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.exinfo->prikeyPeriod,-1);
      else if(strcmp(value,"证书名称约束")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.exinfo->nameConstraints,-1);
      else if(strcmp(value,"证书方针映射")==0)
         gtk_tree_store_set (tree_store, &iter_v ,0,certificate.exinfo->polMappings,-1);
      else 
         gtk_tree_store_set (tree_store, &iter_v ,0,NULL,-1); 
      value=NULL;
   }
}

static void export_certificates(GtkWidget *widget) 
{
   GtkWidget *dialog,*dialog1;
   GtkFileFilter *filter;
   gint result,result1;
   char *pathname=NULL;
   char *pretermitname=NULL;
   char *filevalue=NULL;
   GTlsCertificate* certificate1=NULL;
   GByteArray *certificateData=NULL;
   int i;
   FILE *fp, *fp1;
   int file_size;
   char pem[DER_MAX_SIZE]={0};

   dialog=gtk_file_chooser_dialog_new ("保存证书至文件", NULL,
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                         NULL);

   if(!ce_chain_value)
   {
      filter=gtk_file_filter_new(); 
      gtk_file_filter_set_name (filter, "X.509 证书(PKCS#12)");
      gtk_file_filter_add_pattern(filter,"*.[Pp][11][11]");
      gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog),filter); 
 
      filter=gtk_file_filter_new();
      gtk_file_filter_set_name (filter, "所有文件");
      gtk_file_filter_add_pattern(filter,"*");
      gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog),filter);
      while(1)
      {
         result=gtk_dialog_run (GTK_DIALOG (dialog));
         if (result==GTK_RESPONSE_ACCEPT)  
         {  
            filter=gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog));
            pathname=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            if(g_file_test(pathname,G_FILE_TEST_EXISTS))
            {
               dialog1=gtk_message_dialog_new((GtkWindow *)dialog,
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_QUESTION,
                                              GTK_BUTTONS_YES_NO,
                                                                                                                          "文件名已经存在，是否想要替换?\n替换该文件将覆盖其中的内容.");
               result1=gtk_dialog_run(GTK_DIALOG (dialog1));
               if(result1==GTK_RESPONSE_YES)
               {
                  gtk_widget_destroy (dialog1);
                  break;
               }
               else
               {
                  pathname=NULL;
                  gtk_widget_destroy (dialog1);
                  continue; 
               }
            }
            else
               break;
         }
         else
         {
            gtk_widget_destroy (dialog);
            return;
         }
      }
      fp=fopen(export_personal_path,"r");
      fseek(fp,0,SEEK_END);   
      file_size=ftell(fp);
      fseek(fp,0,SEEK_SET);
      fread(pem,file_size,1,fp);
      fp1=fopen(pathname,"w");
      fwrite(pem,file_size,1,fp1);
      fclose(fp);
      fclose(fp1);
      gtk_widget_destroy (dialog);
      return;
   }
   filter=gtk_file_filter_new();
   gtk_file_filter_set_name (filter, "X.509 证书(PEM)");
   gtk_file_filter_add_pattern(filter,"*.[Pp][Ee][Mm]");
   gtk_file_filter_add_pattern(filter,"*.[Cc][Rr][Tt]");
   gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog),filter);

   filter=gtk_file_filter_new();
   gtk_file_filter_set_name (filter, "X.509 含链证书(PEM)");
   gtk_file_filter_add_pattern(filter,"*.[Pp][Ee][Mm]");
   gtk_file_filter_add_pattern(filter,"*.[Cc][Rr][Tt]");
   gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog),filter);
 
   filter=gtk_file_filter_new();
   gtk_file_filter_set_name (filter, "X.509 证书(DER)");
   gtk_file_filter_add_pattern(filter,"*.[Dd][Ee][Rr]");
   gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog),filter);

   filter=gtk_file_filter_new();
   gtk_file_filter_set_name (filter, "X.509 证书(PKCS#7)");
   gtk_file_filter_add_pattern(filter,"*.[Pp][77][Cc]");
   gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog),filter);

   filter=gtk_file_filter_new(); 
   gtk_file_filter_set_name (filter, "X.509 含链证书(PKCS#7)");
   gtk_file_filter_add_pattern(filter,"*.[Pp][77][Cc]");
   gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog),filter); 

   filter=gtk_file_filter_new();
   gtk_file_filter_set_name (filter, "所有文件");
   gtk_file_filter_add_pattern(filter,"*");
   gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog),filter);

   gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),getenv("HOME"));

   //set pretermit file name
   if(!certificate.to->commonName)
   {
      if(certificate.to->organizationalUnitName)
      {
         pretermitname=(char *)malloc(strlen(certificate.to->organizationalUnitName)+1);
         strcpy(pretermitname,certificate.to->organizationalUnitName);
      }
      else
      {
         pretermitname=(char *)malloc(strlen(certificate.to->organizationName)+1);
         strcpy(pretermitname,certificate.to->organizationName);
      }
   }
   else
      get_subject_name(ce_chain_value,&pretermitname);     
   gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER (dialog),pretermitname);

   while(1)
   {
      result=gtk_dialog_run (GTK_DIALOG (dialog));
      if (result==GTK_RESPONSE_ACCEPT)  
      {  
         filter=gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog));
         pathname=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
         if(g_file_test(pathname,G_FILE_TEST_EXISTS))
         {
           dialog1=gtk_message_dialog_new((GtkWindow *)dialog,
                              GTK_DIALOG_MODAL,
                              GTK_MESSAGE_QUESTION,
                              GTK_BUTTONS_YES_NO,
                              "文件名已经存在，是否想要替换?\n替换该文件将覆盖其中的内容.");
            result1=gtk_dialog_run(GTK_DIALOG (dialog1));
            if(result1==GTK_RESPONSE_YES)
            {
               gtk_widget_destroy (dialog1);
               break;
            }
            else
            {
               pathname=NULL;
               gtk_widget_destroy (dialog1);
               continue; 
            }
         }
         else
            break;
      }
      else
      {
         free(pretermitname);
         pretermitname=NULL;
         gtk_widget_destroy (dialog);
         return;
      }
   }
   if(strcmp(gtk_file_filter_get_name(filter),"X.509 证书(PEM)")==0 ||
      strcmp(gtk_file_filter_get_name(filter),"所有文件")==0)
      g_file_set_contents(pathname,ce_chain_value,strlen(ce_chain_value),NULL);
   else if(strcmp(gtk_file_filter_get_name(filter),"X.509 含链证书(PEM)")==0)
   {
      if(!ce_chain_value1)
      {
         filevalue=(char *)malloc(strlen(ce_chain_value)+1);
         strcpy(filevalue,ce_chain_value);
      }
      else if(!ce_chain_value2)
      {
         filevalue=(char *)malloc(strlen(ce_chain_value)+strlen(ce_chain_value1)+1);
         strcpy(filevalue,ce_chain_value);
         strcat(filevalue,ce_chain_value1);           
      }
      else
      {
         filevalue=(char *)malloc(strlen(ce_chain_value)+strlen(ce_chain_value1)+strlen(ce_chain_value2)+1);
         strcpy(filevalue,ce_chain_value);
         strcat(filevalue,ce_chain_value1);
         strcat(filevalue,ce_chain_value2);   
      }
      g_file_set_contents(pathname,filevalue,strlen(filevalue),NULL);
      free(filevalue);
      filevalue=NULL;
   }
   else if(strcmp(gtk_file_filter_get_name(filter),"X.509 证书(DER)")==0)
   {
      certificate1=g_tls_certificate_new_from_pem(
           ce_chain_value, strlen(ce_chain_value)+1, NULL);
      g_object_get(G_OBJECT(certificate1), "certificate", &certificateData, NULL);
      filevalue=(char *)malloc(certificateData->len+1);
      for(i=0;i<(int)certificateData->len;i++)
         filevalue[i]=(char)certificateData->data[i];
      filevalue[i]='\0';
      g_file_set_contents(pathname,filevalue,certificateData->len,NULL);
      free(filevalue);
      filevalue=NULL;
      pathname=NULL;
      certificate1=NULL;
      certificateData=NULL;
   }
   free(pretermitname);
   pretermitname=NULL;
   gtk_widget_destroy (dialog);
}

static int close_single_certificate_window()
{
   free_single_certificate_memeory();
   free(ce_chain_value);
   ce_chain_value=NULL;
   free(ce_chain_value1);
   ce_chain_value1=NULL;
   free(ce_chain_value2);
   ce_chain_value2=NULL;
   glade_num=0;
   free(export_personal_path);
   export_personal_path = NULL;
   return false;
}

//display single certificate information
void display_single_certificate()
{
   char title[CERTITLELENGTH]={0};
   GtkLabel *label;
   GtkButton *export_button;
   GtkTreeSelection *selection,*selection1;
   GtkTreeIter   iter;
   GtkTreeIter   iter_child,iter_child1;
   GtkTreeViewColumn *column;
   char *verfity_value=NULL;
   char *certificate_chain[CHAIN_MAX_SIZE]={NULL,NULL,NULL};
   char certificateglade_path[CERTITLELENGTH]={0};

   if(!glade_num)
   {
      certificate_builder=gtk_builder_new();
      strcat(certificateglade_path, midori_paths_get_res_dir());    
      strcat(certificateglade_path,"/layout/single-certificate.glade");
      gtk_builder_add_from_file(certificate_builder,certificateglade_path, NULL);
      certificate_window=(GtkWindow *)GTK_WIDGET(gtk_builder_get_object(certificate_builder,"window"));
  
      scrolledwindow=GTK_WIDGET(gtk_builder_get_object(certificate_builder,"scrolledwindow"));
      scrolledwindow1=GTK_WIDGET(gtk_builder_get_object(certificate_builder,"scrolledwindow1"));
      scrolledwindow2=GTK_WIDGET(gtk_builder_get_object(certificate_builder,"scrolledwindow2"));
      export_button=(GtkButton *)GTK_WIDGET(gtk_builder_get_object (certificate_builder,"button2"));

      tree1=gtk_tree_view_new();
      tree_store1=gtk_tree_store_new(1,G_TYPE_STRING);
      renderer1=gtk_cell_renderer_text_new();
      gtk_container_add(GTK_CONTAINER (scrolledwindow),(GtkWidget *)tree1);

      tree2=gtk_tree_view_new();
      tree_store2=gtk_tree_store_new(1,G_TYPE_STRING);
      renderer2=gtk_cell_renderer_text_new();
      gtk_container_add(GTK_CONTAINER (scrolledwindow1),(GtkWidget *)tree2);

      tree3=gtk_tree_view_new();
      tree_store3=gtk_tree_store_new(1,G_TYPE_STRING);
      renderer3=gtk_cell_renderer_text_new ();
      gtk_container_add(GTK_CONTAINER (scrolledwindow2),(GtkWidget *)tree3);     
   }
   else
      gtk_window_present(GTK_WINDOW(certificate_window));

   //set title
   strcpy(title,"证书查看器：");
   if(certificate.to->commonName)
   {
      if(strcmp(certificate.to->commonName,certificate.by->commonName) ==0)
      {
         strcat(title,"Builtin Object Token:");
         strcat(title,certificate.to->commonName);
      }
      else 
         strcat(title,certificate.to->commonName);
   }   
   else if (certificate.to->organizationalUnitName)
   {
      strcat(title,"Builtin Object Token:");
      strcat(title,certificate.to->organizationalUnitName);
   }
   else
   {
      strcat(title,"Builtin Object Token:");
      strcat(title,certificate.to->organizationName);      
   }
   gtk_window_set_title(certificate_window,title);
   memset(title,0,CERTITLELENGTH);   
   
   //get certificate chain
   get_certificate_chain(certificate_chain);

   //check certificate verfity
   check_certificate_verfity(&verfity_value);

   //output basic information
   label=(GtkLabel *)GTK_WIDGET(gtk_builder_get_object(certificate_builder,"verfity"));   
   if(verfity_value)
   {
       sprintf(title,"此证书没有通过认证,没有通过的原因为:%s",verfity_value);
       gtk_label_set_text(label,title);
       memset(title,0,CERTITLELENGTH);
       verfity_value=NULL;
   }
   else
      gtk_label_set_text(label,"此证书已经过安全认证.");
   if(certificate.to->commonName)
   {
      label=(GtkLabel *)GTK_WIDGET(gtk_builder_get_object(certificate_builder,"ToCommonName"));
      gtk_label_set_text(label,certificate.to->commonName);
   }
   if(certificate.to->organizationName)
   {
      label =  (GtkLabel *)GTK_WIDGET (gtk_builder_get_object (certificate_builder, "ToOrgName"));
      gtk_label_set_text(label,g_locale_to_utf8(certificate.to->organizationName,-1,0,0,0));
   }
   if(certificate.to->organizationalUnitName)
   {
      label=(GtkLabel *)GTK_WIDGET(gtk_builder_get_object(certificate_builder,"ToOrgUnitName"));
      gtk_label_set_text(label,g_locale_to_utf8(certificate.to->organizationalUnitName,-1,0,0,0));
   }
   label=(GtkLabel *)GTK_WIDGET(gtk_builder_get_object(certificate_builder,"SerialNumber"));
   gtk_label_set_text(label,certificate.serialNumber);
   if(certificate.by->commonName)
   {
      label=(GtkLabel *)GTK_WIDGET(gtk_builder_get_object(certificate_builder,"ByCommonName"));
      gtk_label_set_text(label,certificate.by->commonName);
   }
   if(certificate.by->organizationName)
   {
      label=(GtkLabel *)GTK_WIDGET(gtk_builder_get_object(certificate_builder,"ByOrgName"));
      gtk_label_set_text(label,g_locale_to_utf8(certificate.by->organizationName,-1,0,0,0));
   }
   if(certificate.by->organizationalUnitName)
   {
      label=(GtkLabel *)GTK_WIDGET(gtk_builder_get_object(certificate_builder,"ByOrgUnitName"));
      gtk_label_set_text(label,g_locale_to_utf8(certificate.by->organizationalUnitName,-1,0,0,0));
   }
   label=(GtkLabel *)GTK_WIDGET(gtk_builder_get_object(certificate_builder,"StartDate"));
   gtk_label_set_text(label,certificate.startDate);
   label=(GtkLabel *)GTK_WIDGET(gtk_builder_get_object(certificate_builder,"EndDate"));
   gtk_label_set_text(label,certificate.endDate);
   label=(GtkLabel *)GTK_WIDGET(gtk_builder_get_object(certificate_builder,"Sha1"));
   gtk_label_set_text(label,certificate.sha1_t);
   label=(GtkLabel *)GTK_WIDGET(gtk_builder_get_object(certificate_builder,"Md5"));
   gtk_label_set_text(label,certificate.md5_t);

   //output detail infomation
   gtk_tree_store_clear(tree_store1);
   if(certificate_chain[CHAIN_MAX_SIZE-1])
   {
      gtk_tree_store_append(tree_store1, &iter, NULL); 
      gtk_tree_store_set(tree_store1, &iter,0,certificate_chain[CHAIN_MAX_SIZE-1],-1);
      free(certificate_chain[CHAIN_MAX_SIZE-1]);
      certificate_chain[CHAIN_MAX_SIZE-1]=NULL;
      gtk_tree_store_append(tree_store1,&iter_child,&iter);
      gtk_tree_store_set(tree_store1,&iter_child,0,certificate_chain[CHAIN_MAX_SIZE-2],-1);
      free(certificate_chain[CHAIN_MAX_SIZE-2]);
      certificate_chain[CHAIN_MAX_SIZE-2]=NULL;
      gtk_tree_store_append(tree_store1,&iter_child1,&iter_child);
      gtk_tree_store_set(tree_store1, &iter_child1,0,certificate_chain[CHAIN_MAX_SIZE-3],-1);
      free(certificate_chain[CHAIN_MAX_SIZE-3]);
      certificate_chain[CHAIN_MAX_SIZE-3]=NULL;
   }    
   else if(!certificate_chain[CHAIN_MAX_SIZE-1] && certificate_chain[CHAIN_MAX_SIZE-2])
   {
      gtk_tree_store_append(tree_store1,&iter,NULL); 
      gtk_tree_store_set(tree_store1, &iter,0,certificate_chain[CHAIN_MAX_SIZE-2],-1);
      free(certificate_chain[CHAIN_MAX_SIZE-2]);
      certificate_chain[CHAIN_MAX_SIZE-2]=NULL;
      gtk_tree_store_append(tree_store1, &iter_child,&iter);
      gtk_tree_store_set(tree_store1,&iter_child,0,certificate_chain[CHAIN_MAX_SIZE-3],-1);
      free(certificate_chain[CHAIN_MAX_SIZE-3]);
      certificate_chain[CHAIN_MAX_SIZE-3]=NULL;
   }
   else
   {
      gtk_tree_store_append(tree_store1, &iter, NULL); 
      gtk_tree_store_set(tree_store1,&iter,0,certificate_chain[CHAIN_MAX_SIZE-3],-1);
      free(certificate_chain[CHAIN_MAX_SIZE-3]);
      certificate_chain[CHAIN_MAX_SIZE-3]=NULL; 
   } 
   if(!glade_num)
   {
      gtk_tree_view_set_model(GTK_TREE_VIEW(tree1),GTK_TREE_MODEL(tree_store1));       
      column=gtk_tree_view_column_new_with_attributes("证书结构",renderer1,"text",0,NULL);
      gtk_tree_view_append_column(GTK_TREE_VIEW(tree1),column);
   }
   gtk_tree_view_expand_all((GtkTreeView *)tree1);
   gtk_widget_show(tree1);

   get_tree_store(tree_store2);
   if(!glade_num)
   {
      gtk_tree_view_set_model(GTK_TREE_VIEW(tree2),GTK_TREE_MODEL(tree_store2));
      column=gtk_tree_view_column_new_with_attributes("证书字段",renderer2,"text",0,NULL);
      gtk_tree_view_append_column(GTK_TREE_VIEW (tree2),column);
   }
   gtk_tree_view_expand_all((GtkTreeView *)tree2);
   gtk_widget_show(tree2);
    
   gtk_tree_store_clear(tree_store3); 
   gtk_tree_store_append(tree_store3, &iter_v,NULL);
   gtk_tree_store_set(tree_store3,&iter_v,0,NULL,-1);
   if(!glade_num)
   {    
      gtk_tree_view_set_model(GTK_TREE_VIEW(tree3),GTK_TREE_MODEL(tree_store3));           
      column=gtk_tree_view_column_new_with_attributes("字段值",renderer3,"text",0,NULL); 
      gtk_tree_view_append_column(GTK_TREE_VIEW(tree3),column);
   }
   gtk_widget_show(tree3);

   selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(tree1));
   //根据证书结构中的值改变证书字段的值（单个证书详细信息） 
   g_signal_connect(selection,"changed", G_CALLBACK(on_changed_segment),tree_store2);

   selection1=gtk_tree_view_get_selection(GTK_TREE_VIEW (tree2));
   //根据证书字段中的值改变字段值（单个证书详细信息）
   g_signal_connect(selection1,"changed",G_CALLBACK(on_changed_segment_value),tree_store3);

   //export certificate information
   if(glade_num==0)
      g_signal_connect(export_button,"clicked",G_CALLBACK(export_certificates),NULL);   
  
   glade_num=1;

   //close window
   g_signal_connect(G_OBJECT(certificate_window),"delete_event", G_CALLBACK(close_single_certificate_window),NULL);
}

static void set_manager_certificate(char *root_cerpath,GtkListStore *store)
{
   GtkTreeIter iter;
   DIR *dir=NULL;
   struct dirent *ptr=NULL;
   BIO *b=NULL;
   X509 *x=NULL;
   char *path=NULL;
   FILE *fp;
   int file_size;
   char pem[DER_MAX_SIZE]={0};
   char *p=NULL;
   char *device=NULL;
   char *byname=NULL;
   char *toname=NULL;
   char *name=NULL;
   char *tmp=NULL;
   int j;
   char date[TIMELENGTH]={0};
   X509_NAME *issuer,*subject;
   int entriesNum,Nid;
   X509_NAME_ENTRY *name_entry; 

   if((dir=opendir(root_cerpath))==NULL)
      return;   
   while((ptr=readdir(dir))!=NULL)
   {
      if((strcmp(ptr->d_name,".")==0) || (strcmp(ptr->d_name,"..")==0))
         continue;
      path=(char *)malloc(strlen(ptr->d_name)+strlen(root_cerpath)+1);
      strcpy(path,root_cerpath);
      strcat(path,ptr->d_name);
      b=BIO_new_file(path,"r");   
      x=PEM_read_bio_X509(b,NULL,NULL,NULL);
      BIO_free(b);
      if(!x)
      {
         fp=fopen(path,"r");
         fseek(fp,0,SEEK_END);   
         file_size=ftell(fp);
         fseek(fp,0,SEEK_SET);
         fread(pem,file_size,1,fp);
         p=pem;
         x=d2i_X509(NULL,(const unsigned char ** )&p,file_size);
         fclose(fp);
         if(!x)
         { 
            //only support der or pem
            X509_free(x);
            free(path);
            path=NULL;
            memset(pem,0,sizeof(pem));
            continue;
         }
         memset(pem,0,sizeof(pem));
      }
      free(path);
      path=NULL;      
      tmp=strrchr(ptr->d_name,'.');
      name=(char *)malloc(strlen(ptr->d_name)+1);
      if(tmp)
      {
         memcpy(name,ptr->d_name,tmp-ptr->d_name);
         name[tmp-ptr->d_name]='\0';
      }
      else
         strcpy(name,ptr->d_name);
      for(j=0;name[j];j++)
         if(name[j]=='_')
            name[j]=' ';      
      issuer=X509_get_issuer_name(x); 
      entriesNum=sk_X509_NAME_ENTRY_num(issuer->entries);
      for(j=0;j<entriesNum;j++)
      {
         name_entry=sk_X509_NAME_ENTRY_value(issuer->entries,j);
         Nid=OBJ_obj2nid(name_entry->object);
         if(Nid==NID_commonName)
         {
            byname=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(byname,name_entry->value->data);
            break;
         }
      }
      subject=X509_get_subject_name(x);
      entriesNum=sk_X509_NAME_ENTRY_num(subject->entries);
      for(j=0;j<entriesNum;j++)
      {
         name_entry=sk_X509_NAME_ENTRY_value(subject->entries,j);
         Nid=OBJ_obj2nid(name_entry->object);
         if(Nid==NID_commonName)
         { 
            toname=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(toname,name_entry->value->data);    
            break;
         }
      }
      if(toname)
      {
         if(strcmp(byname,toname)==0)
         {
            device=(char *)malloc(strlen("Builtin Object Token")+1);
            strcpy(device,"Builtin Object Token");   
         }
         else
         {
            device=(char *)malloc(strlen("软件安全设备")+1);
            strcpy(device,"软件安全设备");  
         }
         free(byname);
         free(toname);
         byname=NULL;
         toname=NULL;
      }
      else
      {
         device=(char *)malloc(strlen("Builtin Object Token")+1);
         strcpy(device,"Builtin Object Token");
      }     
      gtk_list_store_append(store,&iter);
      gtk_list_store_set(store,&iter,
                         0,name,
                         1,device,
                         2,asn1_gettime(X509_get_notAfter(x),date),
                         -1);
      X509_free(x);
      memset(date,0,sizeof(date));
      free(device);
      device=NULL;
      free(name);
      name=NULL;
   }
}

static void get_manager_personal_certificate(char *root_path,GtkListStore *store)
{
   GtkTreeIter iter;
   DIR *dir = NULL;
   BIO *bio = NULL;
   X509 *x = NULL;
   PKCS12 *pk12 = NULL;
   EVP_PKEY* pkey = NULL;  
   STACK_OF(X509)* ca = NULL;
   struct dirent *ptr=NULL;
   char *path = NULL;
   char *password = NULL;
   char date[TIMELENGTH]={0};

   if((dir=opendir(root_path))==NULL)
      return;
   while((ptr=readdir(dir))!=NULL)
   {
      if((strcmp(ptr->d_name,".")==0) || (strcmp(ptr->d_name,"..")==0))
         continue;
      password = certificate_secret_password_lookup (ptr->d_name);
      if(!password) continue;                
      path=(char *)malloc(strlen(ptr->d_name)+strlen(root_path)+1);
      strcpy(path,root_path);
      strcat(path,ptr->d_name);
      SSLeay_add_all_algorithms();  
      ERR_load_crypto_strings();
      bio=BIO_new_file(path,"r");
      pk12 = d2i_PKCS12_bio(bio, NULL);
      BIO_free_all(bio);
      PKCS12_parse (pk12, password, &pkey, &x, &ca);
      PKCS12_free(pk12);
      free(path);
      path=NULL;
      if(!x)
         continue;
      gtk_list_store_append(store,&iter);
      gtk_list_store_set(store,&iter,
                         0,ptr->d_name,
                         1,asn1_gettime(X509_get_notAfter(x),date),
                                                                   -1);
      X509_free(x);
      memset(date,0,sizeof(date));
     }
}
//设置证书管理器的您的证书信息
static GtkTreeModel* set_manager_personal_certificate()
{
   GtkListStore *store;
   char root_cerpath[CERTITLELENGTH]={0};
 
   store=gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_STRING);
   strcpy(root_cerpath,getenv("HOME"));
   strcat(root_cerpath,"/.cache/webkitgtk/personal_certificate/");
   get_manager_personal_certificate(root_cerpath,store);
   return GTK_TREE_MODEL(store);
}

//设置证书管理器的授权中心中证书信息
static GtkTreeModel* set_manager_root_certificate()
{
   GtkListStore *store;
   char root_cerpath[CERTITLELENGTH]={0};
 
   store=gtk_list_store_new(3,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
   strcpy(root_cerpath,midori_paths_get_res_dir());
   strcat(root_cerpath,"/root_certificate/");
   set_manager_certificate(root_cerpath,store);
   strcpy(root_cerpath,getenv("HOME"));
   strcat(root_cerpath,"/.cache/webkitgtk/root_certificate/");
   set_manager_certificate(root_cerpath,store);
   return GTK_TREE_MODEL(store);
}

static bool set_single_certificate_info()
{
   int i;
   DIR *dir=NULL;
   struct dirent *ptr=NULL;
   char *path=NULL;
   char *value=NULL;
   BIO *b=NULL;
   BIO *out=NULL;
   X509 *x=NULL;
   FILE *fp;
   int file_size;
   char pem[DER_MAX_SIZE]={0};
   char *p=NULL;
   char root_cerpath[CERTITLELENGTH]={0};

   strcpy(root_cerpath,midori_paths_get_res_dir());
   strcat(root_cerpath,"/root_certificate/");
   if((dir=opendir(root_cerpath))!=NULL)
   {
      while((ptr=readdir(dir))!=NULL)
      {
         if(strncmp(ptr->d_name,root_name,strlen(root_name))==0)
         {
            path=(char *)malloc(strlen(ptr->d_name)+strlen(root_cerpath)+1);
            strcpy(path,root_cerpath);
            strcat(path,ptr->d_name);
            break;
         }
      }
   }
   if(!path)
   {
      strcpy(root_cerpath,getenv("HOME"));
      strcat(root_cerpath,"/.cache/webkitgtk/root_certificate/");
      if((dir=opendir(root_cerpath))!=NULL)
      {
         while((ptr=readdir(dir))!=NULL)
         {
            if(strncmp(ptr->d_name,root_name,strlen(root_name))==0)
            {
               path=(char *)malloc(strlen(ptr->d_name)+strlen(root_cerpath)+1);
               strcpy(path,root_cerpath);
               strcat(path,ptr->d_name);
               break;
            }
         }
      }
   }
   if(!path)
      return false;
   b=BIO_new_file(path,"r");   
   x=PEM_read_bio_X509(b,NULL,NULL,NULL);
   if(x)
   {
      fp=fopen(path,"r");
      fseek(fp,0,SEEK_END);   
      file_size=ftell(fp);
      fseek(fp,0,SEEK_SET);
      fread(pem,file_size,1,fp);
      fclose(fp);
      p=pem;
      for(i=0;;p++,i++)
      {
         if(*p=='E'&&strncmp(p,"END CERTIFICATE",strlen("END CERTIFICATE"))==0)
         break;
      }
      value=(char *)malloc(i+1+strlen("END CERTIFICATE-----"));
      memcpy(value,pem,i+strlen("END CERTIFICATE-----"));
      value[i+strlen("END CERTIFICATE-----")]='\0'; 
      memset(pem,0,sizeof(pem)); 
   }
   else
   {
      b=BIO_new_file(path,"r");
      x=d2i_X509_bio(b,NULL);
      if(!x)
      {
         BIO_free(b);
         free(path);
         path=NULL; 
         return false; 
      }
      out=BIO_new(BIO_s_mem());
      PEM_write_bio_X509(out,x);
      BIO_read(out,pem,DER_MAX_SIZE);
      BIO_free(out);
      p=pem;
      for(i=0;;p++,i++)
      {
         if(*p=='E'&&strncmp(p,"END CERTIFICATE",strlen("END CERTIFICATE"))==0)
         break;
      }
      value=(char *)malloc(i+1+strlen("END CERTIFICATE-----"));
      memcpy(value,pem,i+strlen("END CERTIFICATE-----"));
      value[i+strlen("END CERTIFICATE-----")]='\0';
      memset(pem,0,sizeof(pem));
   }
   BIO_free(b);
   X509_free(x);
   free(path);
   path=NULL;
   get_single_certificate_data(value);
   free(value);
   value=NULL;
   return true;
}

static bool set_personal_single_certificate_info()
{
   DIR *dir = NULL;
   struct dirent *ptr = NULL;
   char *path = NULL;
   char root_cerpath[CERTITLELENGTH] = {0};

   strcpy(root_cerpath,getenv("HOME"));
   strcat(root_cerpath,"/.cache/webkitgtk/personal_certificate/");
   if((dir=opendir(root_cerpath))!=NULL)
   {
      while((ptr=readdir(dir))!=NULL)
      {
         if(strcmp(ptr->d_name,personal_name)==0)
         {
            path=(char *)malloc(strlen(ptr->d_name)+strlen(root_cerpath)+1);
            strcpy(path,root_cerpath);
            strcat(path,ptr->d_name);
            certificate_password = certificate_secret_password_lookup (ptr->d_name);
            break;
         }
      }
   }
   if(!path || !certificate_password)
      return false;
   get_single_certificate_data(path);
   free(path);
   path=NULL;
   return true;
}

//证书管理器中您的证书的查看按钮:显示单个证书详情
static void manager_showpersonal_certificate(GtkWidget *widget)
{
   bool check=false;

   check=set_personal_single_certificate_info();
   if(check)
      display_single_certificate();       
}

//证书管理器中授权中心的查看按钮:显示单个证书详情
static void show_root_certificate(GtkWidget *widget)
{
   bool check=false;

   check=set_single_certificate_info();
   if(check)
      display_single_certificate();       
}

//点击证书管理器中您的证书中的证书
static void get_personal_certificate_name(GtkWidget *widget)
{
   GtkTreeIter iter;
   GtkTreeModel *model; 
   DIR *dir=NULL;

   if(gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter))
   {
      gtk_tree_model_get(model,&iter,0,&personal_name,-1);
      gtk_widget_set_sensitive((GtkWidget *)personal_delete_button,TRUE);
      gtk_widget_set_sensitive((GtkWidget *)personal_show_button,TRUE);
//      gtk_widget_set_sensitive((GtkWidget *)personal_export_button,TRUE);
   }
}

//点击证书管理器中授权中心中的证书
static void get_root_certificate_name(GtkWidget *widget)
{
   GtkTreeIter iter;
   GtkTreeModel *model; 
   DIR *dir=NULL;
   char root_cerpath[CERTITLELENGTH]={0};
   struct dirent *ptr=NULL;
   int i;

   if(gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter))
   {
      gtk_tree_model_get(model,&iter,0,&root_name,-1);
      gtk_widget_set_sensitive((GtkWidget *)manager_rootdelete_button,false);
      gtk_widget_set_sensitive((GtkWidget *)manager_rootshow_button,TRUE);
      gtk_widget_set_sensitive((GtkWidget *)manager_rootexport_button,TRUE);
      for(i=0;root_name[i];i++)
         if(root_name[i]==' ')
            root_name[i]='_';
      strcpy(root_cerpath,getenv("HOME"));
      strcat(root_cerpath,"/.cache/webkitgtk/root_certificate/");
      if((dir=opendir(root_cerpath))!=NULL)
      {
         while((ptr=readdir(dir))!=NULL)
         {
            if(strncmp(ptr->d_name,root_name,strlen(root_name))==0)
               gtk_widget_set_sensitive((GtkWidget *)manager_rootdelete_button,TRUE);
         }
      }     
   }
}

//检查导入的证书是否已存在
static bool check_root_certificate(char *root_path,char *byname)
{
   DIR *dir=NULL;
   BIO *b=NULL;
   X509 *x=NULL;
   X509_NAME *issuer;
   int entriesNum,Nid;
   X509_NAME_ENTRY *name_entry;
   struct dirent *ptr=NULL;
   FILE *fp;
   int file_size;
   char pem[DER_MAX_SIZE]={0};
   char *p=NULL;
   int j;
   char *path=NULL;
   
   if((dir=opendir(root_path))==NULL)
      return false;
   while((ptr=readdir(dir))!=NULL)
   {
      if((strcmp(ptr->d_name,".")==0) || (strcmp(ptr->d_name,"..")==0))
         continue;
      path=(char *)malloc(strlen(ptr->d_name)+strlen(root_path)+1);
      strcpy(path,root_path);
      strcat(path,ptr->d_name);
      b=BIO_new_file(path,"r");  
      x=PEM_read_bio_X509(b,NULL,NULL,NULL);
      BIO_free(b);
      if(!x)
      {
         fp=fopen(path,"r");
         fseek(fp,0,SEEK_END);   
         file_size=ftell(fp);
         fseek(fp,0,SEEK_SET);
         fread(pem,file_size,1,fp);
         fclose(fp);
         p=pem;
         x=d2i_X509(NULL,(const unsigned char ** )&p,file_size);
         if(!x)
         { 
            //only support der or pem
            free(path);
            path=NULL;
            memset(pem,0,sizeof(pem));
            continue;
         }
         memset(pem,0,sizeof(pem));
      }
      free(path);
      path=NULL;
      issuer=X509_get_issuer_name(x); 
      entriesNum=sk_X509_NAME_ENTRY_num(issuer->entries);
      for(j=0;j<entriesNum;j++)
      {
         name_entry=sk_X509_NAME_ENTRY_value(issuer->entries,j);
         Nid=OBJ_obj2nid(name_entry->object);
         if(Nid==NID_commonName)
         {
            if(strcmp(byname,name_entry->value->data)==0)
            {
               X509_free(x);
               return true;  
            }
         }
         if(Nid==NID_organizationName)
         {
            if(strcmp(byname,name_entry->value->data)==0)
            {
               X509_free(x);
               return true;  
            }
         }
         if(Nid==NID_organizationalUnitName)
         {
            if(strcmp(byname,name_entry->value->data)==0)
            {
               X509_free(x);
               return true;  
            }
         }
      }
      X509_free(x);
   }
   return false;
} 

//检查您的证书中导入的证书是否已存在
static bool check_personal_certificate(char *root_path,char *byname,char *toname)
{
   DIR *dir = NULL;
   BIO *bio = NULL;
   X509 *x = NULL;
   PKCS12 *pk12 = NULL;
   EVP_PKEY* pkey = NULL;  
   STACK_OF(X509)* ca = NULL;
   X509_NAME *issuer,*subject;
   int entriesNum,Nid;
   X509_NAME_ENTRY *name_entry;
   struct dirent *ptr=NULL;
   int i;
   char *path = NULL;
   char *password = NULL;
   char *toname1 = NULL;
   char *byname1 = NULL;

   if((dir=opendir(root_path))==NULL)
      return false;
   while((ptr=readdir(dir))!=NULL)
   {
      if((strcmp(ptr->d_name,".")==0) || (strcmp(ptr->d_name,"..")==0))
         continue;
      password = certificate_secret_password_lookup (ptr->d_name);
      if(!password) continue;                
      path=(char *)malloc(strlen(ptr->d_name)+strlen(root_path)+1);
      strcpy(path,root_path);
      strcat(path,ptr->d_name);
      SSLeay_add_all_algorithms();  
      ERR_load_crypto_strings();
      bio=BIO_new_file(path,"r");
      pk12 = d2i_PKCS12_bio(bio, NULL);
      BIO_free_all(bio);
      PKCS12_parse (pk12, password, &pkey, &x, &ca);
      PKCS12_free(pk12);
      free(path);
      path=NULL;
      if(!x)
         continue;
      subject=X509_get_subject_name(x);
      entriesNum=sk_X509_NAME_ENTRY_num(subject->entries);
      for(i=0;i<entriesNum;i++)
      {
         name_entry=sk_X509_NAME_ENTRY_value(subject->entries,i);
         Nid=OBJ_obj2nid(name_entry->object);
         if(Nid==NID_commonName)
         { 
            toname1=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(toname1,name_entry->value->data);
            break;    
         }
      }
      issuer=X509_get_issuer_name(x); 
      entriesNum=sk_X509_NAME_ENTRY_num(issuer->entries);
      for(i=0;i<entriesNum;i++)
      {
         name_entry=sk_X509_NAME_ENTRY_value(issuer->entries,i);
         Nid=OBJ_obj2nid(name_entry->object);
         if(Nid==NID_commonName)
         {
            byname1=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(byname1,name_entry->value->data);
            break;
         }
      }
      X509_free(x);
      if(!byname1)
         continue;
      if (strcmp(g_locale_to_utf8(byname1,-1,0,0,0),g_locale_to_utf8(byname,-1,0,0,0)) == 0 &&
          strcmp(g_locale_to_utf8(toname1,-1,0,0,0),g_locale_to_utf8(toname,-1,0,0,0)) == 0)
      {
         free(byname1);
         free(toname1);
         byname1 = toname1 = NULL;
         return true;
      }
   }
   return false;
}

static void on_button_clicked_password (GtkWidget* button,gpointer data)  
{  
   GtkWidget *dialog;
   gint result;
   X509 *x=NULL;
   EVP_PKEY* pkey = NULL;  
   STACK_OF(X509)* ca = NULL;
   X509_NAME *issuer,*subject;
   int entriesNum,Nid;
   X509_NAME_ENTRY *name_entry;
   int i;
   char *byname=NULL;
   char *toname=NULL;
   char personal_cerpath[CERTITLELENGTH]={0};
   bool check=false;
   FILE *fp, *fp1;
   int file_size;
   char pem[DER_MAX_SIZE]={0};
   char *p12_password = NULL;
   DIR *dir=NULL;
   struct dirent *ptr=NULL;

   if((int)data == 1)
   {    
      PKCS12_free(p12);
      personal_certificate_path=NULL;
      gtk_widget_destroy(GTK_WIDGET(popup_password_dialog)); 
      gtk_widget_destroy (personal_import_dialog); 
   } 
   else if ((int)data == 2)
   {    
      char *password = gtk_entry_get_text(GTK_ENTRY(entry));
      p12_password = (char *)malloc(strlen(password)+1);
      strcpy(p12_password,password);
      PKCS12_parse (p12, password, &pkey, &x, &ca);
      PKCS12_free(p12);
      gtk_widget_destroy(GTK_WIDGET(popup_password_dialog));
      if(!x)
      {
         dialog=gtk_message_dialog_new((GtkWindow *)personal_import_dialog,
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK,
                                        "密码输入错误.\n无法导入文件.");
         result=gtk_dialog_run(GTK_DIALOG (dialog));
         if(result==GTK_RESPONSE_OK || result==GTK_RESPONSE_DELETE_EVENT)
         {
            personal_certificate_path=NULL;
            free(p12_password);
            p12_password = NULL;
            gtk_widget_destroy (dialog);
            gtk_widget_destroy (personal_import_dialog);
            return;
         }
      }
      subject=X509_get_subject_name(x);
      entriesNum=sk_X509_NAME_ENTRY_num(subject->entries);
      for(i=0;i<entriesNum;i++)
      {
         name_entry=sk_X509_NAME_ENTRY_value(subject->entries,i);
         Nid=OBJ_obj2nid(name_entry->object);
         if(Nid==NID_commonName)
         { 
            toname=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(toname,name_entry->value->data);
            break;    
         }
      }
      issuer=X509_get_issuer_name(x); 
      entriesNum=sk_X509_NAME_ENTRY_num(issuer->entries);
      for(i=0;i<entriesNum;i++)
      {
         name_entry=sk_X509_NAME_ENTRY_value(issuer->entries,i);
         Nid=OBJ_obj2nid(name_entry->object);
         if(Nid==NID_commonName)
         {
            byname=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(byname,name_entry->value->data);
            break;
         }
      }
      X509_free(x);
      if(byname)
      {
         strcpy(personal_cerpath,getenv("HOME"));
         strcat(personal_cerpath,"/.cache/webkitgtk/personal_certificate/");
         check=check_personal_certificate(personal_cerpath,byname,toname);            
         free(byname);
         byname=NULL;
         free(toname);
         toname = NULL;
      }
      else
      {
         dialog=gtk_message_dialog_new((GtkWindow *)personal_import_dialog,
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK,
                                        "颁发者未知，无法导入.");
         result=gtk_dialog_run(GTK_DIALOG (dialog));
         if(result==GTK_RESPONSE_OK || result==GTK_RESPONSE_DELETE_EVENT)
         {
            personal_certificate_path=NULL;
            free(p12_password);
            p12_password = NULL;
            gtk_widget_destroy (dialog);
            gtk_widget_destroy (personal_import_dialog);
            return;
         }             
      }
      if(check)
      {
         dialog=gtk_message_dialog_new((GtkWindow *)personal_import_dialog,
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK,
                                        "此证书已安装为您的证书.");
         result=gtk_dialog_run(GTK_DIALOG (dialog));
         if(result==GTK_RESPONSE_OK || result==GTK_RESPONSE_DELETE_EVENT)
         {
            personal_certificate_path=NULL;
            free(p12_password);
            p12_password = NULL;
            gtk_widget_destroy (dialog);
            gtk_widget_destroy (personal_import_dialog);
            return;
         }             
      }
      dialog=gtk_message_dialog_new((GtkWindow *)personal_import_dialog,
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_OK_CANCEL,
                                    "确定要导入该证书吗?");
      result=gtk_dialog_run(GTK_DIALOG (dialog));
      if(result==GTK_RESPONSE_OK)
      {
         fp=fopen(personal_certificate_path,"r");
         if(!fp)
         {
            free(p12_password);
            p12_password = NULL;
            personal_certificate_path=NULL;
            gtk_widget_destroy (dialog);
            gtk_widget_destroy (personal_import_dialog);
            return;
         }
         fseek(fp,0,SEEK_END);   
         file_size=ftell(fp);
         fseek(fp,0,SEEK_SET);
         if(file_size<sizeof(pem))
            fread(pem,file_size,1,fp);
         if(!strlen(pem))
         {
            fclose(fp);
            personal_certificate_path=NULL;
            free(p12_password);
            p12_password = NULL;
            gtk_widget_destroy (dialog);
            gtk_widget_destroy (personal_import_dialog);
            return;
         }
         if(!g_file_test(personal_cerpath, G_FILE_TEST_IS_DIR) || !g_access(personal_cerpath, 0755))
            g_mkdir_with_parents(personal_cerpath,0755);
         char *filename = strrchr(personal_certificate_path,'/');
         filename++;
         //如果重名，后导入的名字改成s_文件名。
         dir=opendir(personal_cerpath);
         while((ptr=readdir(dir))!=NULL)
         {
            if((strcmp(ptr->d_name,".")==0) || (strcmp(ptr->d_name,"..")==0))
               continue;
            if(strcmp(filename,ptr->d_name)==0)
            {
               filename =g_strdup_printf ("s_%s",filename);
               dir=opendir(personal_cerpath);
            }
         } 
         strcat(personal_cerpath,filename);
         fp1=fopen(personal_cerpath,"w");
         if(!fp1)
         {
            fclose(fp);
            personal_certificate_path=NULL;
            free(p12_password);
            p12_password = NULL;
            gtk_widget_destroy (dialog);
            gtk_widget_destroy (personal_import_dialog);
            return;
         }
         fwrite(pem,file_size,1,fp1);
         fclose(fp);
         fclose(fp1);                        
         personal_certificate_path=NULL;
         //保存密码
         certificate_secret_password_removed(filename);
         certificate_secret_password_store(filename,p12_password);
         free(p12_password);
         p12_password = NULL;
         gtk_widget_destroy (dialog);
         gtk_widget_destroy (personal_import_dialog);
         return;                          
      }
      else
      {
         personal_certificate_path=NULL;
         gtk_widget_destroy (dialog);
         gtk_widget_destroy (personal_import_dialog);
         return;
      }      
   }
}  

static void popup_password_dialog_function(void)
{
   char certificateglade_path[CERTITLELENGTH]={0};
  
   GtkBuilder *certificate_builder=gtk_builder_new();
   strcat(certificateglade_path, midori_paths_get_res_dir());    
   strcat(certificateglade_path,"/layout/get_password.glade");
   gtk_builder_add_from_file(certificate_builder,certificateglade_path, NULL); 
   popup_password_dialog = GTK_WIDGET (gtk_builder_get_object (certificate_builder, "dialog1"));
   GtkWidget* label1 = GTK_WIDGET (gtk_builder_get_object (certificate_builder, "label1"));
   gtk_label_set_text((GtkLabel *)label1,"请输入用于加密该证书的密码：");
   GtkWidget* label2 = GTK_WIDGET (gtk_builder_get_object (certificate_builder, "label2"));
   gtk_label_set_text((GtkLabel *)label2,"密码：");
   entry = GTK_WIDGET (gtk_builder_get_object (certificate_builder, "entry1"));
   gtk_entry_set_visibility(GTK_ENTRY(entry),FALSE);
   GtkWidget* button1 = GTK_WIDGET (gtk_builder_get_object (certificate_builder, "button1"));
   GtkWidget* button2 = GTK_WIDGET (gtk_builder_get_object (certificate_builder, "button2"));  
   g_signal_connect(G_OBJECT(button1),"clicked",G_CALLBACK(on_button_clicked_password),(gpointer)1);   
   g_signal_connect(G_OBJECT(button2),"clicked",G_CALLBACK(on_button_clicked_password),(gpointer)2); 
   gint result=gtk_dialog_run (GTK_DIALOG (popup_password_dialog));
   if(result==GTK_RESPONSE_DELETE_EVENT)
        {
      personal_certificate_path=NULL;
      gtk_widget_destroy(GTK_WIDGET(popup_password_dialog)); 
      gtk_widget_destroy (personal_import_dialog);
        }
   gtk_builder_connect_signals (certificate_builder, NULL);
   g_object_unref (G_OBJECT (certificate_builder));   
}

//证书管理器的您的证书导入按钮:导入证书
static void manager_personalimport_certificates(GtkWidget *widget)
{
   GtkWidget *dialog;
   GtkFileFilter *filter;
   gint result,result1;
   BIO *bio = NULL;

   personal_import_dialog=gtk_file_chooser_dialog_new ("请选择要导入的包含CA证书的文件", NULL,
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                         NULL);
   filter=gtk_file_filter_new();
   gtk_file_filter_set_name (filter, "X.509 证书(P12)");
   gtk_file_filter_add_pattern(filter,"*.[Pp][11][22]");
   gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (personal_import_dialog),filter);

   filter=gtk_file_filter_new();
   gtk_file_filter_set_name (filter, "所有文件");
   gtk_file_filter_add_pattern(filter,"*");
   gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (personal_import_dialog),filter);

   gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (personal_import_dialog),getenv("HOME"));
   result=gtk_dialog_run (GTK_DIALOG (personal_import_dialog));
   if(result==GTK_RESPONSE_ACCEPT)  
        {  
      filter=gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(personal_import_dialog));
      personal_certificate_path=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(personal_import_dialog));
      SSLeay_add_all_algorithms();  
      ERR_load_crypto_strings();
      bio=BIO_new_file(personal_certificate_path,"r");
      p12 = d2i_PKCS12_bio(bio, NULL);
      BIO_free_all(bio);
      if (p12)
         popup_password_dialog_function();          
      else
                {
         dialog=gtk_message_dialog_new((GtkWindow *)personal_import_dialog,
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK,
                                                                                                        "证书导入错误.\n无法解析文件.");
         result1=gtk_dialog_run(GTK_DIALOG (dialog));
         if(result1==GTK_RESPONSE_OK || result1==GTK_RESPONSE_DELETE_EVENT)
                        {
            personal_certificate_path=NULL;
            gtk_widget_destroy (dialog);
            gtk_widget_destroy (personal_import_dialog);
                        }   
                }
        }
   else
      gtk_widget_destroy (personal_import_dialog);        
}

//证书管理器的您的证书删除按钮:删除证书
static void manager_personaldelete_certificates(GtkWidget *widget)
{
   DIR *dir=NULL;
   char root_cerpath[CERTITLELENGTH]={0};
   struct dirent *ptr=NULL;
   GtkWidget *dialog;
   gint result;

   strcpy(root_cerpath,getenv("HOME"));
   strcat(root_cerpath,"/.cache/webkitgtk/personal_certificate/");
   if((dir=opendir(root_cerpath))!=NULL)
   {
      while((ptr=readdir(dir))!=NULL)
      {
         if(strcmp(ptr->d_name,personal_name)==0)
         {
            strcat(root_cerpath,ptr->d_name);
            dialog=gtk_message_dialog_new(NULL,
                         GTK_DIALOG_MODAL,
                         GTK_MESSAGE_QUESTION,
                         GTK_BUTTONS_OK_CANCEL,
                         "确定要删除该证书吗?\n\n如果您删除了你自己的证书,\n你将不能再通过该证书来识别你的身份.");
            result=gtk_dialog_run(GTK_DIALOG (dialog));
            if(result==GTK_RESPONSE_OK)
                                {
              remove(root_cerpath);
              certificate_secret_password_removed(ptr->d_name);
                                }   
            gtk_widget_destroy (dialog);          
            break;
         }
      }
   }
}

//证书管理器的授权中心删除按钮:删除证书
static void manager_rootdelete_certificates(GtkWidget *widget)
{
   DIR *dir=NULL;
   char root_cerpath[CERTITLELENGTH]={0};
   struct dirent *ptr=NULL;
   GtkWidget *dialog;
   gint result;

   strcpy(root_cerpath,getenv("HOME"));
   strcat(root_cerpath,"/.cache/webkitgtk/root_certificate/");
   if((dir=opendir(root_cerpath))!=NULL)
   {
      while((ptr=readdir(dir))!=NULL)
      {
         if(strncmp(ptr->d_name,root_name,strlen(root_name))==0)
         {
            strcat(root_cerpath,ptr->d_name);
            dialog=gtk_message_dialog_new(NULL,
                         GTK_DIALOG_MODAL,
                         GTK_MESSAGE_QUESTION,
                         GTK_BUTTONS_OK_CANCEL,
                         "确定要删除该证书吗?\n\n如果您删除了某个证书授权中心(CA)的证书,\n则浏览器不会再信任该CA颁发的任何证书.");
            result=gtk_dialog_run(GTK_DIALOG (dialog));
            if(result==GTK_RESPONSE_OK)
               remove(root_cerpath);   
            gtk_widget_destroy (dialog);          
            break;
         }
      }
   }
}


//证书管理器的授权中心导入按钮:导入证书
static void manager_rootimport_certificates(GtkWidget *widget)
{
   GtkWidget *dialog,*dialog1;
   GtkFileFilter *filter;
   gint result,result1;
   char *pathname=NULL;
   BIO *b=NULL;
   X509 *x=NULL;
   FILE *fp, *fp1;
   int file_size;
   char pem[DER_MAX_SIZE]={0};
   char *p=NULL;
   X509_NAME *issuer,*subject;
   int entriesNum,Nid;
   X509_NAME_ENTRY *name_entry;
   int i;
   char *byname=NULL;
   char *by_orgname=NULL;
   char *by_orgunitname=NULL;
   char *toname=NULL;
   char root_cerpath[CERTITLELENGTH]={0};
   bool check=false;
   char ch;
   dialog=gtk_file_chooser_dialog_new ("请选择要导入的包含CA证书的文件", NULL,
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                         NULL);
   filter=gtk_file_filter_new();
   gtk_file_filter_set_name (filter, "X.509 证书(PEM)");
   gtk_file_filter_add_pattern(filter,"*.[Pp][Ee][Mm]");
   gtk_file_filter_add_pattern(filter,"*.[Cc][Rr][Tt]");
   gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog),filter);

   filter=gtk_file_filter_new();
   gtk_file_filter_set_name (filter, "X.509 含链证书(PEM)");
   gtk_file_filter_add_pattern(filter,"*.[Pp][Ee][Mm]");
   gtk_file_filter_add_pattern(filter,"*.[Cc][Rr][Tt]");
   gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog),filter);
 
   filter=gtk_file_filter_new();
   gtk_file_filter_set_name (filter, "X.509 证书(DER)");
   gtk_file_filter_add_pattern(filter,"*.[Dd][Ee][Rr]");
   gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog),filter);

   filter=gtk_file_filter_new();
   gtk_file_filter_set_name (filter, "X.509 证书(PKCS#7)");
   gtk_file_filter_add_pattern(filter,"*.[Pp][77][Cc]");
   gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog),filter);

   filter=gtk_file_filter_new(); 
   gtk_file_filter_set_name (filter, "X.509 含链证书(PKCS#7)");
   gtk_file_filter_add_pattern(filter,"*.[Pp][77][Cc]");
   gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog),filter);

   filter=gtk_file_filter_new();
   gtk_file_filter_set_name (filter, "所有文件");
   gtk_file_filter_add_pattern(filter,"*");
   gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog),filter);

   gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),getenv("HOME"));

   result=gtk_dialog_run (GTK_DIALOG (dialog));
   if(result==GTK_RESPONSE_ACCEPT)  
   {  
      filter=gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog));
      pathname=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
      b=BIO_new_file(pathname,"r");   
      x=PEM_read_bio_X509(b,NULL,NULL,NULL);
      BIO_free(b);
      if(!x)
      {
         fp=fopen(pathname,"r");  
         fseek(fp,0,SEEK_END);   
         file_size=ftell(fp);
         fseek(fp,0,SEEK_SET);
         if(file_size<sizeof(pem))
         {
            fread(pem,file_size,1,fp);
            p=pem;
            x=d2i_X509(NULL,(const unsigned char ** )&p,file_size);
         }
         fclose(fp);
      }
      if(!x)
      {
         dialog1=gtk_message_dialog_new((GtkWindow *)dialog,
                              GTK_DIALOG_MODAL,
                              GTK_MESSAGE_ERROR,
                              GTK_BUTTONS_OK,
                              "证书导入错误.\n无法解析文件.");
         result1=gtk_dialog_run(GTK_DIALOG (dialog1));
         if(result1==GTK_RESPONSE_OK || result1==GTK_RESPONSE_DELETE_EVENT)
         {
            pathname=NULL;
            gtk_widget_destroy (dialog1);
            gtk_widget_destroy (dialog);
            return;
         }   
      }         
      subject=X509_get_subject_name(x);
      entriesNum=sk_X509_NAME_ENTRY_num(subject->entries);
      for(i=0;i<entriesNum;i++)
      {
         name_entry=sk_X509_NAME_ENTRY_value(subject->entries,i);
         Nid=OBJ_obj2nid(name_entry->object);
         if(Nid==NID_commonName)
         { 
            toname=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(toname,name_entry->value->data);
            break;    
         }
      }
      issuer=X509_get_issuer_name(x); 
      entriesNum=sk_X509_NAME_ENTRY_num(issuer->entries);
      for(i=0;i<entriesNum;i++)
      {
         name_entry=sk_X509_NAME_ENTRY_value(issuer->entries,i);
         Nid=OBJ_obj2nid(name_entry->object);
         if(Nid==NID_commonName)
         {
            byname=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(byname,name_entry->value->data);
         }
         if(Nid==NID_organizationName)
         {
            by_orgname=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(by_orgname,name_entry->value->data);
         }
         if(Nid==NID_organizationalUnitName)
         {
            by_orgunitname=(char *)malloc(strlen(name_entry->value->data)+1);
            strcpy(by_orgunitname,name_entry->value->data);
         }
      }
      X509_free(x);
      if(byname && strcmp(byname,toname)!=0)
      {         
         dialog1=gtk_message_dialog_new((GtkWindow *)dialog,
                           GTK_DIALOG_MODAL,
                           GTK_MESSAGE_ERROR,
                           GTK_BUTTONS_OK,
                           "这不是一个证书授权中心证书,因此无法导入到证书授权中心列表.");
         result1=gtk_dialog_run(GTK_DIALOG (dialog1));
         if(result1==GTK_RESPONSE_OK || result1==GTK_RESPONSE_DELETE_EVENT)
         {
            pathname=NULL;
            gtk_widget_destroy (dialog1);
            gtk_widget_destroy (dialog);
            return;
         }    
      }
      if(byname)
      {
         p=(char *)malloc(strlen(toname)+1);
         strcpy(p,toname);
         free(by_orgname);
         by_orgname=NULL;
         free(by_orgunitname);
         by_orgunitname=NULL;
         strcpy(root_cerpath,midori_paths_get_res_dir());
         strcat(root_cerpath,"/root_certificate/");
         check=check_root_certificate(root_cerpath,byname);
         if(!check)
         {
            strcpy(root_cerpath,getenv("HOME"));
            strcat(root_cerpath,"/.cache/webkitgtk/root_certificate/");
            check=check_root_certificate(root_cerpath,byname);            
         }
         free(byname);
         byname=NULL;
         free(toname);
         toname=NULL; 
      }
      else
      {
         if(by_orgname)
         {
            p=(char *)malloc(strlen(by_orgname)+1);
            strcpy(p,by_orgname);
            if(by_orgunitname)
            {
               free(by_orgunitname);
               by_orgunitname=NULL;
            }
            strcpy(root_cerpath,midori_paths_get_res_dir());
            strcat(root_cerpath,"/root_certificate/");
            check=check_root_certificate(root_cerpath,by_orgname);
            if(!check)
            {
               strcpy(root_cerpath,getenv("HOME"));
               strcat(root_cerpath,"/.cache/webkitgtk/root_certificate/");
               check=check_root_certificate(root_cerpath,by_orgname);            
            }
            free(by_orgname);
            by_orgname=NULL;
         }
         else
         {
            p=(char *)malloc(strlen(by_orgunitname)+1);
            strcpy(p,by_orgunitname);
            strcpy(root_cerpath,midori_paths_get_res_dir());
            strcat(root_cerpath,"/root_certificate/");
            check=check_root_certificate(root_cerpath,by_orgunitname);
            if(!check)
            {
               strcpy(root_cerpath,getenv("HOME"));
               strcat(root_cerpath,"/.cache/webkitgtk/root_certificate/");
               check=check_root_certificate(root_cerpath,by_orgunitname);            
            }
            free(by_orgunitname);
            by_orgunitname=NULL;          
         }
      }
      if(check)
      {
         dialog1=gtk_message_dialog_new((GtkWindow *)dialog,
                           GTK_DIALOG_MODAL,
                           GTK_MESSAGE_ERROR,
                           GTK_BUTTONS_OK,
                           "此证书已安装为证书授权中心.");
         result1=gtk_dialog_run(GTK_DIALOG (dialog1));
         if(result1==GTK_RESPONSE_OK || result1==GTK_RESPONSE_DELETE_EVENT)
         {
            pathname=NULL;
            free(p);
            p=NULL;
            gtk_widget_destroy (dialog1);
            gtk_widget_destroy (dialog);
            return;
         }             
      }
      dialog1=gtk_message_dialog_new((GtkWindow *)dialog,
                           GTK_DIALOG_MODAL,
                           GTK_MESSAGE_QUESTION,
                           GTK_BUTTONS_OK_CANCEL,
                           "确定要导入该证书吗?");
      result1=gtk_dialog_run(GTK_DIALOG (dialog1));
      if(result1==GTK_RESPONSE_OK)
      {
         strcpy(root_cerpath,getenv("HOME"));
         strcat(root_cerpath,"/.cache/webkitgtk/root_certificate");
         if(!g_file_test(root_cerpath, G_FILE_TEST_IS_DIR) || !g_access(root_cerpath, 0755))
            g_mkdir_with_parents(root_cerpath,0755);
         strcat(root_cerpath,"/");
         strcat(root_cerpath,p);
         strcat(root_cerpath,".crt");
         free(p);
         p=NULL;
         fp=fopen(pathname,"r");
         fp1=fopen(root_cerpath,"w");
         if(strlen(pem))
            fwrite(pem,file_size,1,fp1);
         else
         {
            while(!feof(fp))
            {
               ch=fgetc(fp);
               if(ch==EOF)
                  break;
               fputc(ch,fp1);
            }
         }
         fclose(fp);
         fclose(fp1);
         pathname=NULL;
         gtk_widget_destroy (dialog1);
      }
      else
      {
         pathname=NULL;
         free(p);
         p=NULL;
         gtk_widget_destroy (dialog1);
      }      
   }
   gtk_widget_destroy (dialog);
}

//证书管理器的授权中心导出按钮:导出证书
static void manager_rootexport_certificates(GtkWidget *widget)
{
   char *value=NULL;
   bool check;

   if(ce_chain_value)
   {
      value=(char *)malloc(strlen(ce_chain_value)+1);
      strcpy(value,ce_chain_value);
   }
   check=set_single_certificate_info();
   if(check)
      export_certificates(widget);
  //同时打开查看和导出，导出后，恢复查看信息．
   if(value)
   {
      get_single_certificate_data(value);   
      free(value);
      value=NULL;
   }
   //否则，释放
   else
   {
      free_single_certificate_memeory();
      free(ce_chain_value);
      ce_chain_value=NULL;
      free(ce_chain_value1);
      ce_chain_value1=NULL;
      free(ce_chain_value2);
      ce_chain_value2=NULL;
   }
}

//证书管理器的授权中心确定按钮:关闭证书管理器窗口
static void close_certificate_manager(GtkButton *button,GtkWindow *window)
{
   root_name=NULL;
   personal_name=NULL;
   gtk_widget_destroy(GTK_WIDGET(manager_window));
   manager_window=NULL;
}

//证书管理器窗口右上角的Ｘ:关闭证书管理器窗口
static int close_certificate_manager1()
{
   root_name=NULL;
   personal_name=NULL;
   manager_window=NULL;
   return false;
}

//display certificate manager information
void display_certificatemanager()
{
   GtkBuilder *builder;
   GtkWidget *scrolledwindow1, *scrolledwindow2, *scrolledwindow3, *scrolledwindow4;
   GtkWidget *list1, *list2, *list3, *list4;
   GtkListStore *store2, *store4;
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;
   GtkTreeSelection *selection, *selection1;
   GtkButton *ok_button, *ok_button1, *ok_button2, *ok_button3;
   GtkButton *manager_rootimport_button;
   GtkButton *personal_import_button;
   GtkButton *server_show_button, *server_export_button, *server_import_button, *server_delete_button;
   GtkButton *other_show_button, *other_export_button, *other_delete_button;
   char managerglade_path[CERTITLELENGTH]={0};

   if(manager_window) 
   {
      gtk_window_present(GTK_WINDOW(manager_window));
      return;
   }
   builder=gtk_builder_new();
   strcat(managerglade_path,midori_paths_get_res_dir());
   strcat(managerglade_path,"/layout/certificate_manager.glade");
   gtk_builder_add_from_file(builder,managerglade_path, NULL);

   manager_window=(GtkWindow *)GTK_WIDGET(gtk_builder_get_object(builder,"window"));
   scrolledwindow1=GTK_WIDGET(gtk_builder_get_object(builder,"scrolledwindow1"));
   scrolledwindow2=GTK_WIDGET(gtk_builder_get_object(builder,"scrolledwindow2"));
   scrolledwindow3=GTK_WIDGET(gtk_builder_get_object(builder,"scrolledwindow3"));
   scrolledwindow4=GTK_WIDGET(gtk_builder_get_object(builder,"scrolledwindow4"));
   
   //您的证书
   list1=gtk_tree_view_new_with_model(set_manager_personal_certificate());
   personal_show_button=(GtkButton *)GTK_WIDGET(gtk_builder_get_object (builder,"button1"));
   personal_export_button=(GtkButton *)GTK_WIDGET(gtk_builder_get_object (builder,"button3"));
   personal_import_button=(GtkButton *)GTK_WIDGET(gtk_builder_get_object (builder,"button2"));
   personal_delete_button=(GtkButton *)GTK_WIDGET(gtk_builder_get_object (builder,"button4"));
   gtk_widget_set_sensitive((GtkWidget *)personal_show_button,FALSE);
   gtk_widget_set_sensitive((GtkWidget *)personal_export_button,FALSE);
   gtk_widget_set_sensitive((GtkWidget *)personal_delete_button,FALSE);
   renderer=gtk_cell_renderer_text_new();
   column=gtk_tree_view_column_new_with_attributes("证书名称     ",renderer,"text",0,NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(list1), column);
   column=gtk_tree_view_column_new_with_attributes("过期时间",renderer,"text",1,NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(list1), column);
   gtk_container_add(GTK_CONTAINER (scrolledwindow1),list1);
   gtk_tree_view_expand_all((GtkTreeView *)list1);
   gtk_widget_show(list1);
   selection1=gtk_tree_view_get_selection(GTK_TREE_VIEW(list1));
   g_signal_connect(selection1,"changed", G_CALLBACK(get_personal_certificate_name),NULL);
   g_signal_connect(personal_show_button,"clicked",G_CALLBACK(manager_showpersonal_certificate),NULL);
   g_signal_connect(personal_import_button,"clicked",G_CALLBACK(manager_personalimport_certificates),NULL);
 //  g_signal_connect(personal_export_button,"clicked",G_CALLBACK(manager_personalexport_certificates),NULL);
   g_signal_connect(personal_delete_button,"clicked",G_CALLBACK(manager_personaldelete_certificates),NULL);

   //服务器
   store2=gtk_list_store_new(3,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
   list2=gtk_tree_view_new_with_model(GTK_TREE_MODEL(store2));
   server_show_button=(GtkButton *)GTK_WIDGET(gtk_builder_get_object (builder,"button6"));
   server_export_button=(GtkButton *)GTK_WIDGET(gtk_builder_get_object (builder,"button7"));
   server_import_button=(GtkButton *)GTK_WIDGET(gtk_builder_get_object (builder,"button8"));
   server_delete_button=(GtkButton *)GTK_WIDGET(gtk_builder_get_object (builder,"button9"));
   gtk_widget_set_sensitive((GtkWidget *)server_show_button,FALSE);
   gtk_widget_set_sensitive((GtkWidget *)server_export_button,FALSE);
   gtk_widget_set_sensitive((GtkWidget *)server_import_button,FALSE);
   gtk_widget_set_sensitive((GtkWidget *)server_delete_button,FALSE);

   column=gtk_tree_view_column_new_with_attributes("证书名称      ",renderer,"text",0,NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(list2), column);
   column=gtk_tree_view_column_new_with_attributes("服务器        ",renderer,"text",0,NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(list2), column);
   column=gtk_tree_view_column_new_with_attributes("过期时间",renderer,"text",0,NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(list2), column);   
   gtk_container_add (GTK_CONTAINER (scrolledwindow2),list2);
   gtk_tree_view_expand_all((GtkTreeView *)list2);
   gtk_widget_show(list2);

   //授权中心
   list3=gtk_tree_view_new_with_model(set_manager_root_certificate());
   column=gtk_tree_view_column_new_with_attributes("证书名称",renderer,"text",0,NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(list3), column);
      column=gtk_tree_view_column_new_with_attributes("安全设备",renderer,"text",1,NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(list3), column);
   column=gtk_tree_view_column_new_with_attributes("过期时间",renderer,"text",2,NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(list3), column);
   gtk_container_add (GTK_CONTAINER (scrolledwindow3),list3);
   gtk_tree_view_expand_all((GtkTreeView *)list3);
   gtk_widget_show(list3);
   selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(list3));
   manager_rootshow_button=(GtkButton *)GTK_WIDGET(gtk_builder_get_object (builder,"button11"));
   manager_rootimport_button=(GtkButton *)GTK_WIDGET(gtk_builder_get_object (builder,"button12"));
   manager_rootdelete_button=(GtkButton *)GTK_WIDGET(gtk_builder_get_object (builder,"button13"));
   manager_rootexport_button=(GtkButton *)GTK_WIDGET(gtk_builder_get_object (builder,"button15"));
   gtk_widget_set_sensitive((GtkWidget *)manager_rootexport_button,FALSE);
   gtk_widget_set_sensitive((GtkWidget *)manager_rootshow_button,FALSE);
   gtk_widget_set_sensitive((GtkWidget *)manager_rootdelete_button,FALSE);  
   g_signal_connect(selection,"changed", G_CALLBACK(get_root_certificate_name),NULL);    
   g_signal_connect(manager_rootshow_button,"clicked",G_CALLBACK(show_root_certificate),NULL);
   g_signal_connect(manager_rootimport_button,"clicked",G_CALLBACK(manager_rootimport_certificates),NULL);
   g_signal_connect(manager_rootexport_button,"clicked",G_CALLBACK(manager_rootexport_certificates),NULL);
   g_signal_connect(manager_rootdelete_button,"clicked",G_CALLBACK(manager_rootdelete_certificates),NULL);

   
   //其他
   store4=gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_STRING);
   list4=gtk_tree_view_new_with_model(GTK_TREE_MODEL(store4));
   other_show_button=(GtkButton *)GTK_WIDGET(gtk_builder_get_object (builder,"button17"));
   other_export_button=(GtkButton *)GTK_WIDGET(gtk_builder_get_object (builder,"button18"));
   other_delete_button=(GtkButton *)GTK_WIDGET(gtk_builder_get_object (builder,"button19"));
   gtk_widget_set_sensitive((GtkWidget *)other_show_button,FALSE);
   gtk_widget_set_sensitive((GtkWidget *)other_export_button,FALSE);
   gtk_widget_set_sensitive((GtkWidget *)other_delete_button,FALSE);
   column=gtk_tree_view_column_new_with_attributes("证书名称       ",renderer,"text",0,NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(list4), column);
   column=gtk_tree_view_column_new_with_attributes("过期时间",renderer,"text",0,NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(list4), column);
   gtk_container_add (GTK_CONTAINER (scrolledwindow4),list4);
   gtk_tree_view_expand_all((GtkTreeView *)list4);
   gtk_widget_show(list4);

   //close window
   ok_button=(GtkButton*)GTK_WIDGET(gtk_builder_get_object(builder,"button5"));
   g_signal_connect(ok_button,"clicked",G_CALLBACK(close_certificate_manager),NULL);
   ok_button1=(GtkButton*)GTK_WIDGET(gtk_builder_get_object(builder,"button10"));
   g_signal_connect(ok_button1,"clicked",G_CALLBACK(close_certificate_manager),NULL);
   ok_button2=(GtkButton*)GTK_WIDGET(gtk_builder_get_object(builder,"button16"));
   g_signal_connect(ok_button2,"clicked",G_CALLBACK(close_certificate_manager),NULL);
   ok_button3=(GtkButton*)GTK_WIDGET(gtk_builder_get_object(builder,"button20"));
   g_signal_connect(ok_button3,"clicked",G_CALLBACK(close_certificate_manager),NULL);
   g_signal_connect(G_OBJECT(manager_window),"delete_event", G_CALLBACK(close_certificate_manager1),NULL);

 //  gtk_builder_connect_signals(builder,NULL);
 //  g_object_unref(G_OBJECT(builder));
}

