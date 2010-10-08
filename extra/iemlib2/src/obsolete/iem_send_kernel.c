/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib2 written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2004 */

#ifdef _MSC_VER
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#include "m_pd.h"
#include "g_canvas.h"
#include "iemlib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

EXTERN void canvas_getargs(int *argcp, t_atom **argvp);

/* -------------------- iem_send_kernel ------------------------------ */

static t_class *iem_send_kernel_class;

typedef struct _iem_send_kernel
{
  t_object x_obj;
  t_symbol *x_sym;
} t_iem_send_kernel;

static void iem_send_kernel_set_name(t_iem_send_kernel *x, t_symbol *s, int argc, t_atom *argv)
{
  if((argc > 0)&&((IS_A_SYMBOL(argv,0))||(IS_A_FLOAT(argv,0))))
  {
    if(IS_A_SYMBOL(argv,0))
      x->x_sym = atom_getsymbol(argv);
    else if(IS_A_FLOAT(argv,0))
    {
      char str[100];
      
      sprintf(str, "%g", atom_getfloat(argv));
      x->x_sym = gensym(str);
    }
  }
}

static void iem_send_kernel_clear(t_iem_send_kernel *x)
{
  x->x_sym = 0;
}

static void iem_send_kernel_bang(t_iem_send_kernel *x)
{
  if(x->x_sym->s_thing)
    pd_bang(x->x_sym->s_thing);
}

static void iem_send_kernel_float(t_iem_send_kernel *x, t_float f)
{
  if(x->x_sym->s_thing)
    pd_float(x->x_sym->s_thing, f);
}

static void iem_send_kernel_symbol(t_iem_send_kernel *x, t_symbol *s)
{
  if(x->x_sym->s_thing)
    pd_symbol(x->x_sym->s_thing, s);
}

static void iem_send_kernel_pointer(t_iem_send_kernel *x, t_gpointer *gp)
{
  if(x->x_sym->s_thing)
    pd_pointer(x->x_sym->s_thing, gp);
}

static void iem_send_kernel_list(t_iem_send_kernel *x, t_symbol *s, int argc, t_atom *argv)
{
  if(x->x_sym->s_thing)
    pd_list(x->x_sym->s_thing, s, argc, argv);
}

static void iem_send_kernel_anything(t_iem_send_kernel *x, t_symbol *s, int argc, t_atom *argv)
{
  if(x->x_sym->s_thing)
    typedmess(x->x_sym->s_thing, s, argc, argv);
}

static void *iem_send_kernel_new(void)
{
  t_iem_send_kernel *x = (t_iem_send_kernel *)pd_new(iem_send_kernel_class);
  t_glist *glist = (t_glist *)canvas_getcurrent();
  t_canvas *canvas=glist_getcanvas(glist);
  int ac=0;
  t_atom *av;
  
  canvas_setcurrent(canvas);
  canvas_getargs(&ac, &av);
  canvas_unsetcurrent(canvas);
  
  if(ac > 0)
  {
    if(IS_A_SYMBOL(av,0))
    {
      x->x_sym = atom_getsymbol(av);
    }
    else if(IS_A_FLOAT(av,0))
    {
      char str[100];
      
      sprintf(str, "%g", atom_getfloat(av));
      x->x_sym = gensym(str);
    }
    else
      x->x_sym = 0;
  }
  else
    x->x_sym = 0;
  return (x);
}

void iem_send_kernel_setup(void)
{
  char str[2];
  
  str[1] = 0;
  iem_send_kernel_class = class_new(gensym("iem_send_kernel"), (t_newmethod)iem_send_kernel_new, 0,
    sizeof(t_iem_send_kernel), 0, 0);
  class_addcreator((t_newmethod)iem_send_kernel_new, gensym("s"), A_DEFSYM, 0);
  class_addbang(iem_send_kernel_class, iem_send_kernel_bang);
  class_addfloat(iem_send_kernel_class, iem_send_kernel_float);
  class_addsymbol(iem_send_kernel_class, iem_send_kernel_symbol);
  class_addpointer(iem_send_kernel_class, iem_send_kernel_pointer);
  class_addlist(iem_send_kernel_class, iem_send_kernel_list);
  class_addanything(iem_send_kernel_class, iem_send_kernel_anything);
  str[0] = 1;/*inlet-sym = "\0x01"*/
  class_addmethod(iem_send_kernel_class, (t_method)iem_send_kernel_set_name, gensym(str), A_GIMME, 0);
  str[0] = 2;/*inlet-sym = "\0x02"*/
  class_addmethod(iem_send_kernel_class, (t_method)iem_send_kernel_clear, gensym(str), 0);
  class_sethelpsymbol(iem_send_kernel_class, gensym("iemhelp/help-iem_send"));
}
