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

/* ------------------------ iem_receive_kernel ---------------------------- */

static t_class *iem_receive_kernel_class;

typedef struct _iem_receive_kernel
{
  t_object  x_obj;
  t_symbol  *x_sym;
} t_iem_receive_kernel;

void iem_receive_kernel_copy_at(t_atom *src, t_atom *dst, int n)
{
  while(n--)
    *dst++ = *src++;
}

static void iem_receive_kernel_set_name(t_iem_receive_kernel *x, t_symbol *s, int argc, t_atom *argv)
{
  if((argc > 0)&&((IS_A_SYMBOL(argv,0))||(IS_A_FLOAT(argv,0))))
  {
    if(x->x_sym)
      pd_unbind(&x->x_obj.ob_pd, x->x_sym);
    if(IS_A_SYMBOL(argv,0))
      x->x_sym = atom_getsymbol(argv);
    else if(IS_A_FLOAT(argv,0))
    {
      char str[100];
      
      sprintf(str, "%g", atom_getfloat(argv));
      x->x_sym = gensym(str);
    }
    pd_bind(&x->x_obj.ob_pd, x->x_sym);
  }
}

static void iem_receive_kernel_clear(t_iem_receive_kernel *x)
{
  if(x->x_sym)
    pd_unbind(&x->x_obj.ob_pd, x->x_sym);
  x->x_sym = 0;
}

static void iem_receive_kernel_bang(t_iem_receive_kernel *x)
{
  outlet_bang(x->x_obj.ob_outlet);
}

static void iem_receive_kernel_float(t_iem_receive_kernel *x, t_float f)
{
  outlet_float(x->x_obj.ob_outlet, f);
}

static void iem_receive_kernel_symbol(t_iem_receive_kernel *x, t_symbol *s)
{
  outlet_symbol(x->x_obj.ob_outlet, s);
}

static void iem_receive_kernel_pointer(t_iem_receive_kernel *x, t_gpointer *gp)
{
  outlet_pointer(x->x_obj.ob_outlet, gp);
}

static void iem_receive_kernel_list(t_iem_receive_kernel *x, t_symbol *s, int argc, t_atom *argv)
{
  outlet_list(x->x_obj.ob_outlet, &s_list, argc, argv);
}

static void iem_receive_kernel_anything(t_iem_receive_kernel *x, t_symbol *s, int argc, t_atom *argv)
{
  outlet_anything(x->x_obj.ob_outlet, s, argc, argv);
}

static void iem_receive_kernel_free(t_iem_receive_kernel *x)
{
  if(x->x_sym)
    pd_unbind(&x->x_obj.ob_pd, x->x_sym);
}

static void *iem_receive_kernel_new(void)
{
  t_iem_receive_kernel *x = (t_iem_receive_kernel *)pd_new(iem_receive_kernel_class);
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
      pd_bind(&x->x_obj.ob_pd, x->x_sym);
    }
    else if(IS_A_FLOAT(av,0))
    {
      char str[100];
      
      sprintf(str, "%g", atom_getfloat(av));
      x->x_sym = gensym(str);
      pd_bind(&x->x_obj.ob_pd, x->x_sym);
    }
    else
      x->x_sym = 0;
  }
  else
    x->x_sym = 0;
  
  outlet_new(&x->x_obj, &s_list);
  return (x);
}

void iem_receive_kernel_setup(void)
{
  char str[2];
  
  str[1] = 0;
  iem_receive_kernel_class = class_new(gensym("iem_receive_kernel"), (t_newmethod)iem_receive_kernel_new, 
    (t_method)iem_receive_kernel_free, sizeof(t_iem_receive_kernel), 0, 0);
  class_addbang(iem_receive_kernel_class, iem_receive_kernel_bang);
  class_addfloat(iem_receive_kernel_class, iem_receive_kernel_float);
  class_addsymbol(iem_receive_kernel_class, iem_receive_kernel_symbol);
  class_addpointer(iem_receive_kernel_class, iem_receive_kernel_pointer);
  class_addlist(iem_receive_kernel_class, iem_receive_kernel_list);
  class_addanything(iem_receive_kernel_class, iem_receive_kernel_anything);
  //  class_addmethod(iem_receive_kernel_class, (t_method)iem_receive_kernel_set, gensym("set"), A_GIMME, 0);
  str[0] = 1;/*inlet-sym = "\0x01"*/
  class_addmethod(iem_receive_kernel_class, (t_method)iem_receive_kernel_set_name, gensym(str), A_GIMME, 0);
  str[0] = 2;/*inlet-sym = "\0x02"*/
  class_addmethod(iem_receive_kernel_class, (t_method)iem_receive_kernel_clear, gensym(str), 0);
  class_sethelpsymbol(iem_receive_kernel_class, gensym("iemhelp/help-iem_receive"));
}
