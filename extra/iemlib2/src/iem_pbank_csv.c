/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib2 written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2006 */


#include "m_pd.h"
#include "iemlib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>


/* ---------------------------- iem_pbank_csv ------------------------------- */
/* -- is a list storage and management object, can store an array of lists -- */
/* ------------------------------- as an csv file --------------------------- */

/* read and write method needs 2 symbols,
1. symbol is a filename,
2. symbol is a 3 character descriptor

  1.char: 'b'...for blank as ITEM_SEPARATOR (" ")
  1.char: 'c'...for comma as ITEM_SEPARATOR (",")
  1.char: 's'...for semicolon as ITEM_SEPARATOR (";")
  1.char: 't'...for tabulator as ITEM_SEPARATOR ("	" = 0x09)
  
    2.char: 'b'...for blank,return as END_OF_LINE (" \n")
    2.char: 'r'...for return-only as END_OF_LINE ("\n")
    2.char: 's'...for semicolon,return as END_OF_LINE (";\n")
    
      3.char: 'l'...for linux RETURN (0x0A)
      3.char: 'w'...for windows RETURN (0x0D,0x0A)
      3.char: 'm'...for mac RETURN (0x0D)
      
        
          
        change: recall + offset + number
*/

static t_class *iem_pbank_csv_class;

typedef struct _iem_pbank_csv
{
  t_object  x_obj;
  int       x_nr_para;
  int       x_nr_line;
  int       x_line;
  t_atom    *x_atbegmem;
  t_atom    *x_atbegbuf;
  t_atom    *x_atbegout;
  t_canvas  *x_canvas;
  void      *x_list_out;
  void      *x_offset_list_out;
} t_iem_pbank_csv;

static void iem_pbank_csv_write(t_iem_pbank_csv *x, t_symbol *filename, t_symbol *format)
{
  char completefilename[400], eol[4], sep, mode[4], string[200];
  int size, p, l, nrl=x->x_nr_line, nrp=x->x_nr_para;
  int state, max=nrl*nrp, org_size, eollen;
  FILE *fh;
  t_atom *ap=x->x_atbegmem;
  char formattext[100];
  
  strcpy(mode, "bsl"); /*blank-separator, semicolon-return-eol, linux_return*/
  sep = ' ';
  eol[0] = ';';
  eol[1] = 0x0a;
  eol[2] = 0;
  if(filename->s_name[0] == '/')
  {
    strcpy(completefilename, filename->s_name);
  }
  else if(((filename->s_name[0] >= 'A')&&(filename->s_name[0] <= 'Z')||
    (filename->s_name[0] >= 'a')&&(filename->s_name[0] <= 'z'))&&
    (filename->s_name[1] == ':')&&(filename->s_name[2] == '/'))
  {
    strcpy(completefilename, filename->s_name);
  }
  else
  {
    strcpy(completefilename, canvas_getdir(x->x_canvas)->s_name);
    strcat(completefilename, "/");
    strcat(completefilename, filename->s_name);
  }
  
  fh = fopen(completefilename,"wb");
  if(!fh)
  {
    post("iem_pbank_csv_write: cannot create %s !!\n", completefilename);
  }
  else
  {
    if(strlen(format->s_name) >= 3)
    {
      for(p=0; p<3; p++)
      {
        if((format->s_name[p] >= 'A')&&(format->s_name[p] <= 'Z'))
          format->s_name[p] += 'a' - 'A';
      }
      if((format->s_name[0] == 'b')||(format->s_name[0] == 'c')||(format->s_name[0] == 's')||(format->s_name[0] == 't'))
        mode[0] = format->s_name[0];
      if((format->s_name[1] == 'b')||(format->s_name[1] == 'r')||(format->s_name[1] == 's'))
        mode[1] = format->s_name[1];
      if((format->s_name[2] == 'l')||(format->s_name[2] == 'w')||(format->s_name[2] == 'm'))
        mode[2] = format->s_name[2];
    }
    else
      post("iem_pbank_csv_write: use default format %s !!\n", mode);
    
    if(mode[0] == 'b')
    {
      sep = ' ';
      strcpy(formattext, "item-separator = BLANK; ");
    }
    else if(mode[0] == 'c')
    {
      sep = ',';
      strcpy(formattext, "item-separator = COMMA; ");
    }
    else if(mode[0] == 's')
    {
      sep = ';';
      strcpy(formattext, "item-separator = SEMICOLON; ");
    }
    else if(mode[0] == 't')
    {
      sep = 0x09;
      strcpy(formattext, "item-separator = TABULATOR; ");
    }
    
    eollen = 1;
    if(mode[1] == 'b')
    {
      eol[0] = ' ';
      strcat(formattext, "end_of_line_terminator = BLANK-RETURN in ");
    }
    else if(mode[1] == 'r')
    {
      eollen = 0;
      strcat(formattext, "end_of_line_terminator = RETURN in ");
    }
    else if(mode[1] == 's')
    {
      eol[0] = ';';
      strcat(formattext, "end_of_line_terminator = SEMICOLON-RETURN in ");
    }
    
    if(mode[2] == 'l')
    {
      eol[eollen++] = 0x0a;
      strcat(formattext, "LINUX-Format.");
    }
    else if(mode[2] == 'w')
    {
      eol[eollen++] = 0x0d;
      eol[eollen++] = 0x0a;
      strcat(formattext, "WINDOWS-Format.");
    }
    else if(mode[2] == 'm')
    {
      eol[eollen++] = 0x0d;
      strcat(formattext, "MACINTOSH-Format.");
    }
    eol[eollen] = 0;
    
    ap = x->x_atbegmem;
    for(l=0; l<nrl; l++)
    {
      for(p=1; p<nrp; p++)
      {
        if(IS_A_FLOAT(ap, 0))
          fprintf(fh, "%g%c", ap->a_w.w_float, sep);
        else if(IS_A_SYMBOL(ap, 0))
          fprintf(fh, "%s%c", ap->a_w.w_symbol->s_name, sep);
        ap++;
      }
      if(IS_A_FLOAT(ap, 0))
        fprintf(fh, "%g%s", ap->a_w.w_float, eol);
      else if(IS_A_SYMBOL(ap, 0))
        fprintf(fh, "%s%s", ap->a_w.w_symbol->s_name, eol);
      ap++;
    }
    fclose(fh);
    post("iem_pbank_csv: wrote %d parameters x %d lines to file:\n%s\nwith following format:\n%s\n", nrp, nrl, completefilename, formattext);
  }
}

int iem_pbank_csv_text2atom(char *text, int text_size, t_atom **at_beg,
                            int *nalloc, char sep, char eol)
{
  char buf[MAXPDSTRING+1], *bufp, *ebuf = buf+MAXPDSTRING;
  const char *textp = text, *etext = text + text_size;
  int natom = 0;
  t_atom *ap = *at_beg;
  t_float f;
  
  while(1)
  {
    int type;
    
    if(textp == etext)
      break;
    if(*textp == eol)
    {
      SETSEMI(ap);
      textp++;
    }
    else if(*textp == sep)
    {
      SETCOMMA(ap);
      textp++;
    }
    else
    {
      char c;
      int flst = 0, slash = 0, lastslash = 0;
      int firstslash = (*textp == '\\');
      
      bufp = buf;
      do
      {
        c = *bufp = *textp++;
        lastslash = slash;
        slash = (c == '\\');
        
        if (flst >= 0)
        {
          int digit = (c >= '0' && c <= '9'),
            dot = (c == '.'), minus = (c == '-'),
            plusminus = (minus || (c == '+')),
            expon = (c == 'e' || c == 'E');
          if (flst == 0)  /* beginning */
          {
            if (minus) flst = 1;
            else if (digit) flst = 2;
            else if (dot) flst = 3;
            else flst = -1;
          }
          else if (flst == 1) /* got minus */
          {
            if (digit) flst = 2;
            else if (dot) flst = 3;
            else flst = -1;
          }
          else if (flst == 2) /* got digits */
          {
            if (dot) flst = 4;
            else if (expon) flst = 6;
            else if (!digit) flst = -1;
          }
          else if (flst == 3) /* got '.' without digits */
          {
            if (digit) flst = 5;
            else flst = -1;
          }
          else if (flst == 4) /* got '.' after digits */
          {
            if (digit) flst = 5;
            else if (expon) flst = 6;
            else flst = -1;
          }
          else if (flst == 5) /* got digits after . */
          {
            if (expon) flst = 6;
            else if (!digit) flst = -1;
          }
          else if (flst == 6) /* got 'e' */
          {
            if (plusminus) flst = 7;
            else if (digit) flst = 8;
            else flst = -1;
          }
          else if (flst == 7) /* got plus or minus */
          {
            if (digit) flst = 8;
            else flst = -1;
          }
          else if (flst == 8) /* got digits */
          {
            if (!digit) flst = -1;
          }
        }
        if (!slash) bufp++;
      }
      while (textp != etext && bufp != ebuf && *textp != ' ' &&
        (slash || (*textp != sep && *textp != eol)));
      *bufp = 0;
      
      if(*buf == '$' && buf[1] >= '0' && buf[1] <= '9' && !firstslash)
      {
        for (bufp = buf+2; *bufp; bufp++)
          if (*bufp < '0' || *bufp > '9')
          {
            SETDOLLSYM(ap, gensym(buf+1));
            goto iem_pbank_csv_didit;
          }
          SETDOLLAR(ap, atoi(buf+1));
iem_pbank_csv_didit: ;
      }
      else
      {
        if(flst == 2 || flst == 4 || flst == 5 || flst == 8)
        {
          f = atof(buf);
          if((f < 1.0e-20)&&(f > -1.0e-20))
            f = 0.0;
          SETFLOAT(ap, f);
        }
        else
          SETSYMBOL(ap, gensym(buf));
      }
    }
    
    ap++;
    natom++;
    if(natom == *nalloc)
    {
      *at_beg = t_resizebytes(*at_beg, *nalloc * sizeof(t_atom),
        *nalloc * (2*sizeof(t_atom)));
      *nalloc = *nalloc * 2;
      ap = *at_beg + natom;
    }
    if(textp == etext)
      break;
  }
  return(natom);
}

/*static char myq(t_atom *a, int off)
{
char c='0';

  if(IS_A_SEMI(a,off))
  c = 's';
  else if(IS_A_COMMA(a,off))
  c = 'c';
  else if(IS_A_FLOAT(a,off))
    c = 'f';
    else if(IS_A_SYMBOL(a,off))
    c = 'y';
    return(c);
} */

static void iem_pbank_csv_read(t_iem_pbank_csv *x, t_symbol *filename, t_symbol *format)
{
  char completefilename[400], eol[4], sep, mode[4], *txbuf1, *txbuf2, *txvec_src, *txvec_dst;
  int size, p, l, i, j, nrl=x->x_nr_line, nrp=x->x_nr_para, atlen=0;
  int txlen, txalloc, hat_alloc, max, eollen;
  FILE *fh;
  t_atom *ap, *hap, *at;
  char formattext[100];
  
  strcpy(mode, "bsl"); /*blank-separator, semicolon-return-eol, linux_return*/
  sep = ' ';
  eol[0] = ';';
  eol[1] = 0x0a;
  eol[2] = 0;
  if(filename->s_name[0] == '/')/*make complete path + filename*/
  {
    strcpy(completefilename, filename->s_name);
  }
  else if(((filename->s_name[0] >= 'A')&&(filename->s_name[0] <= 'Z')||
    (filename->s_name[0] >= 'a')&&(filename->s_name[0] <= 'z'))&&
    (filename->s_name[1] == ':')&&(filename->s_name[2] == '/'))
  {
    strcpy(completefilename, filename->s_name);
  }
  else
  {
    strcpy(completefilename, canvas_getdir(x->x_canvas)->s_name);
    strcat(completefilename, "/");
    strcat(completefilename, filename->s_name);
  }
  
  fh = fopen(completefilename,"rb");
  if(!fh)
  {
    post("iem_pbank_csv_read: cannot open %s !!\n", completefilename);
  }
  else
  {
    if(strlen(format->s_name) >= 3)
    {
      for(p=0; p<3; p++)
      {
        if((format->s_name[p] >= 'A')&&(format->s_name[p] <= 'Z'))
          format->s_name[p] += 'a' - 'A';
      }
      if((format->s_name[0] == 'b')||(format->s_name[0] == 'c')||(format->s_name[0] == 's')||(format->s_name[0] == 't'))
        mode[0] = format->s_name[0];
      if((format->s_name[1] == 'b')||(format->s_name[1] == 'r')||(format->s_name[1] == 's'))
        mode[1] = format->s_name[1];
      if((format->s_name[2] == 'l')||(format->s_name[2] == 'w')||(format->s_name[2] == 'm'))
        mode[2] = format->s_name[2];
    }
    else
      post("iem_pbank_csv_read: use default format %s !!\n", mode);
    if(mode[0] == 'b')
    {
      sep = ' ';
      strcpy(formattext, "item-separator = BLANK; ");
    }
    else if(mode[0] == 'c')
    {
      sep = ',';
      strcpy(formattext, "item-separator = COMMA; ");
    }
    else if(mode[0] == 's')
    {
      sep = ';';
      strcpy(formattext, "item-separator = SEMICOLON; ");
    }
    else if(mode[0] == 't')
    {
      sep = 0x09;
      strcpy(formattext, "item-separator = TABULATOR; ");
    }
    
    eollen = 1;
    if(mode[1] == 'b')
    {
      eol[0] = ' ';
      strcat(formattext, "end_of_line_terminator = BLANK-RETURN in ");
    }
    else if(mode[1] == 'r')
    {
      eollen = 0;
      strcat(formattext, "end_of_line_terminator = RETURN in ");
    }
    else if(mode[1] == 's')
    {
      eol[0] = ';';
      strcat(formattext, "end_of_line_terminator = SEMICOLON-RETURN in ");
    }
    
    if(mode[2] == 'l')
    {
      eol[eollen++] = 0x0a;
      strcat(formattext, "LINUX-Format.");
    }
    else if(mode[2] == 'w')
    {
      eol[eollen++] = 0x0d;
      eol[eollen++] = 0x0a;
      strcat(formattext, "WINDOWS-Format.");
    }
    else if(mode[2] == 'm')
    {
      eol[eollen++] = 0x0d;
      strcat(formattext, "MACINTOSH-Format.");
    }
    eol[eollen] = 0;
    
    fseek(fh, 0, SEEK_END);
    txalloc = ftell(fh);
    fseek(fh,0,SEEK_SET);
    txbuf1 = (char *)getbytes(2 * txalloc * sizeof(char));
    txbuf2 = (char *)getbytes(2 * txalloc * sizeof(char));
    fread(txbuf1, sizeof(char), txalloc, fh);
    fclose(fh);
    
    txvec_src = txbuf1;
    txvec_dst = txbuf2;
    p = 0;
    for(l=0; l<txalloc; l++)
    {
      if(!strncmp(txvec_src, eol, eollen)) /* replace eol by 0x0a */
      {
        txvec_src += eollen;
        l += eollen - 1;
        *txvec_dst++ = 0x0a;
        p++;
      }
      else if(*txvec_src == sep) /* replace sep by ; */
      {
        txvec_src++;
        *txvec_dst++ = ';';
        p++;
      }
      else if((*txvec_src == '\r')||(*txvec_src == '\n')||(*txvec_src == '\t')) /* remove '\n'-returns */
        txvec_src++;
      else                         /* copy the same char */
      {
        *txvec_dst++ = *txvec_src++;
        p++;
      }
    }
    txlen = p;
    
    txvec_src = txbuf2;
    txvec_dst = txbuf1;
    p = 0;
    for(l=0; l<txlen; l++)
    {
      if((*txvec_src == ';')&&(txvec_src[1] == ';')) /* fill between 2 sep a zero */
      {
        *txvec_dst++ = *txvec_src++;
        *txvec_dst++ = '0';
        p += 2;
      }
      else if((*txvec_src == ';')&&(txvec_src[1] == 0x0a)) /* fill between sep and eol a zero */
      {
        *txvec_dst++ = *txvec_src++;
        *txvec_dst++ = '0';
        p += 2;
      }
      else if((*txvec_src == 0x0a)&&(txvec_src[1] == ';')) /* fill between eol and sep a zero */
      {
        *txvec_dst++ = *txvec_src++;
        *txvec_dst++ = '0';
        p += 2;
      }
      else if(*txvec_src == ',') /* replace a comma by a dot */
      {
        *txvec_dst++ = '.';
        txvec_src++;
        p++;
      }
      else                /* copy the same char */
      {
        *txvec_dst++ = *txvec_src++;
        p++;
      }
    }
    txlen = p;
    
    /*     strncpy(txbuf2, txbuf1, txlen);
    txbuf2[txlen] = 0;
    post("\n\n%s\n\n", txbuf2);   */
    
    hat_alloc = 200;
    hap = t_getbytes(hat_alloc * sizeof(t_atom));
    
    atlen = iem_pbank_csv_text2atom(txbuf1, txlen, &hap, &hat_alloc, ';', 0x0a);
    
    /*   ap = hap;
    i = atlen;
    while(i >= 20)
    {
    post("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",myq(ap,0),myq(ap,1),myq(ap,2),myq(ap,3),myq(ap,4),myq(ap,5),myq(ap,6),myq(ap,7),myq(ap,8),myq(ap,9),myq(ap,10),myq(ap,11),myq(ap,12),myq(ap,13),myq(ap,14),myq(ap,15),myq(ap,16),myq(ap,17),myq(ap,18),myq(ap,19));
    ap += 20;
    i -= 20;
  } */
    
    at = x->x_atbegmem;
    for(l=0; l<nrl; l++)/*reset all*/
    {
      for(p=0; p<nrp; p++)
      {
        SETFLOAT(at, 0.0f);
        at++;
      }
    }
    
    at = x->x_atbegmem;
    ap = hap;
    nrp++;
    i = 0; /* atom-counter */
    j = 0;
    for(l=0; l<nrl; l++)/* nrl line times */
    {
      for(p=1; p<=nrp;)
      {
        if((p == nrp) && !(IS_A_SEMI(ap,0)))
        {
          /*post("too long");*/
          while(!(IS_A_SEMI(ap,0)))
          {
            ap++;
            atlen--;
            /*post("ignore");*/
            j++;
            if(atlen <= 0)
            {
              goto iem_pbank_csv_end;
            }
          }
        }
        else
        {
          if(IS_A_FLOAT(ap,0))
          {
            SETFLOAT(at, ap->a_w.w_float);
            /*post("float");*/
            p++;
            i++;
            at++;
          }
          else if(IS_A_SYMBOL(ap,0))
          {
            SETSYMBOL(at, ap->a_w.w_symbol);
            /*post("sym");*/
            p++;
            i++;
            at++;
          }
          else if(IS_A_SEMI(ap,0))
          {
            /*post("semi");*/
            for(; p<nrp;)
            {
              SETFLOAT(at,0.0);
              /*post("zero");*/
              p++;
              i++;
              at++;
            }
            p=nrp + 1;
          }
          ap++;
          atlen--;
          j++;
        }
        if(atlen <= 0)
        {
          goto iem_pbank_csv_end;
        }
      }
    }
    
iem_pbank_csv_end:
    
    
    freebytes(hap, hat_alloc * sizeof(t_atom));
    freebytes(txbuf1, 2 * txalloc * sizeof(char));
    freebytes(txbuf2, 2 * txalloc * sizeof(char));
    post("iem_pbank_csv: read %d parameters x %d lines from file:\n%s\nwith following format:\n%s\n", nrp-1, nrl, completefilename, formattext);
  }
}

static void iem_pbank_csv_recall(t_iem_pbank_csv *x, t_symbol *s, int ac, t_atom *av)
{
  int i, n, beg=0, nrp=x->x_nr_para;
  t_atom *atbuf=x->x_atbegbuf, *atmem=x->x_atbegmem;
  t_atom *atout=x->x_atbegout;
  
  if(ac >= 2)
    nrp = atom_getintarg(1, ac, av);
  if(ac >= 1)
    beg = atom_getintarg(0, ac, av);
  if(beg < 0)
    beg = 0;
  else if(beg >= x->x_nr_para)
    beg = x->x_nr_para - 1;
  if(nrp < 0)
    nrp = 0;
  else if((beg+nrp) > x->x_nr_para)
    nrp = x->x_nr_para - beg;
  atmem += x->x_nr_para * x->x_line + beg;
  atbuf += beg;
  SETFLOAT(atout, (t_float)beg);
  atout++;
  for(i=0; i<nrp; i++)
  {
    *atbuf++ = *atmem;
    *atout++ = *atmem++;
  }
  outlet_list(x->x_offset_list_out, &s_list, nrp+1, x->x_atbegout);
  outlet_list(x->x_list_out, &s_list, nrp, x->x_atbegout+1);
}

static void iem_pbank_csv_bang(t_iem_pbank_csv *x)
{
  int i, nrp=x->x_nr_para;
  t_atom *atbuf=x->x_atbegbuf;
  t_atom *atout=x->x_atbegout;
  
  SETFLOAT(atout, 0.0f);
  atout++;
  for(i=0; i<nrp; i++)
    *atout++ = *atbuf++;
  outlet_list(x->x_offset_list_out, &s_list, nrp+1, x->x_atbegout);
  outlet_list(x->x_list_out, &s_list, nrp, x->x_atbegout+1);
}

static void iem_pbank_csv_store(t_iem_pbank_csv *x, t_symbol *s, int ac, t_atom *av)
{
  int i, beg=0, nrp=x->x_nr_para;
  t_atom *atbuf=x->x_atbegbuf, *atmem=x->x_atbegmem;
  
  if(ac >= 2)
    nrp = atom_getintarg(1, ac, av);
  if(ac >= 1)
    beg = atom_getintarg(0, ac, av);
  if(beg < 0)
    beg = 0;
  else if(beg >= x->x_nr_para)
    beg = x->x_nr_para - 1;
  if(nrp < 0)
    nrp = 0;
  else if((beg+nrp) > x->x_nr_para)
    nrp = x->x_nr_para - beg;
  atmem += x->x_nr_para * x->x_line;
  atmem += beg;
  atbuf += beg;
  for(i=0; i<nrp; i++)
    *atmem++ = *atbuf++;
}

static void iem_pbank_csv_list(t_iem_pbank_csv *x, t_symbol *s, int ac, t_atom *av)
{
  if(ac >= 2)
  {
    int para_index = atom_getintarg(0, ac, av);
    
    if(para_index >= 0)
    {
      if((para_index+ac-1) <= x->x_nr_para)
      {
        int i;
        
        for(i=1; i<ac; i++)
        {
          x->x_atbegbuf[para_index] = av[i];
          para_index++;
        }
      }
    }
  }
}

static void iem_pbank_csv_ft1(t_iem_pbank_csv *x, t_floatarg fline_nr)
{
  int line = (int)fline_nr;
  
  if(line < 0)
    line = 0;
  else if(line >= x->x_nr_line)
    line = x->x_nr_line - 1;
  x->x_line = line;
}

static void iem_pbank_csv_free(t_iem_pbank_csv *x)
{
  freebytes(x->x_atbegmem, x->x_nr_para * x->x_nr_line * sizeof(t_atom));
  freebytes(x->x_atbegbuf, x->x_nr_para * sizeof(t_atom));
  freebytes(x->x_atbegout, (x->x_nr_para+1) * sizeof(t_atom));
}

static void *iem_pbank_csv_new(t_symbol *s, int ac, t_atom *av)
{
  t_iem_pbank_csv *x = (t_iem_pbank_csv *)pd_new(iem_pbank_csv_class);
  int nrpp=0, nrp=10, nrl=10, p, l, i;
  t_atom *ap;
  
  if((ac >= 1) && IS_A_FLOAT(av,0))
    nrp = atom_getintarg(0, ac, av);
  if((ac >= 2) && IS_A_FLOAT(av,1))
    nrl = atom_getintarg(1, ac, av);
  if(nrp <= 0)
    nrp = 10;
  if(nrl <= 0)
    nrl = 10;
  x->x_line = 0;
  x->x_nr_para = nrp;
  x->x_nr_line = nrl;
  x->x_atbegmem = (t_atom *)getbytes(x->x_nr_para * x->x_nr_line * sizeof(t_atom));
  x->x_atbegbuf = (t_atom *)getbytes(x->x_nr_para * sizeof(t_atom));
  x->x_atbegout = (t_atom *)getbytes((x->x_nr_para+1) * sizeof(t_atom));
  ap = x->x_atbegmem;
  for(l=0; l<nrl; l++)
  {
    for(p=0; p<nrp; p++)
    {
      SETFLOAT(ap, 0.0f);
      ap++;
    }
  }
  ap = x->x_atbegbuf;
  for(p=0; p<nrp; p++)
  {
    SETFLOAT(ap, 0.0f);
    ap++;
  }
  x->x_list_out = outlet_new(&x->x_obj, &s_list);     /*left out*/
  x->x_offset_list_out = outlet_new(&x->x_obj, &s_list);  /*right out*/
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
  x->x_canvas = canvas_getcurrent();
  return (x);
}

/* ---------------- global setup function -------------------- */

void iem_pbank_csv_setup(void )
{
  iem_pbank_csv_class = class_new(gensym("iem_pbank_csv"), (t_newmethod)iem_pbank_csv_new,
    (t_method)iem_pbank_csv_free, sizeof(t_iem_pbank_csv), 0, A_GIMME, 0);
  class_addmethod(iem_pbank_csv_class, (t_method)iem_pbank_csv_recall, gensym("recall"), A_GIMME, 0);
  class_addmethod(iem_pbank_csv_class, (t_method)iem_pbank_csv_store, gensym("store"), A_GIMME, 0);
  class_addmethod(iem_pbank_csv_class, (t_method)iem_pbank_csv_read, gensym("read"), A_SYMBOL, A_DEFSYM, 0);
  class_addmethod(iem_pbank_csv_class, (t_method)iem_pbank_csv_write, gensym("write"), A_SYMBOL, A_DEFSYM, 0);
  class_addlist(iem_pbank_csv_class, iem_pbank_csv_list);
  class_addbang(iem_pbank_csv_class, iem_pbank_csv_bang);
  class_addmethod(iem_pbank_csv_class, (t_method)iem_pbank_csv_ft1, gensym("ft1"), A_FLOAT, 0);
//  class_sethelpsymbol(iem_pbank_csv_class, gensym("iemhelp/help-iem_pbank_csv"));
}
