/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib2 written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2005 */

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

/* ------------------------ iem_anything_kernel ---------------------------- */

static t_class *iem_anything_kernel_class;

typedef struct _iem_anything_kernel
{
  t_object      x_obj;
  int           x_inlet_select;
  int           x_size;
  int           x_ac;
  t_atom        *x_at;
  t_symbol      *x_sym;
} t_iem_anything_kernel;

static void iem_anything_kernel_atcopy(t_atom *src, t_atom *dst, int n)
{
  while(n--)
    *dst++ = *src++;
}

static void iem_anything_kernel_inlet_select(t_iem_anything_kernel *x, t_floatarg in_sel_01)
{
  x->x_inlet_select = (int)in_sel_01;
}

static void iem_anything_kernel_anything(t_iem_anything_kernel *x, t_symbol *s, int ac, t_atom *av)
{
  if((ac == 0)&&(s == &s_bang))
  {
    if(x->x_inlet_select)
    {
      x->x_ac = ac;
      x->x_sym = s;
    }
    else
      outlet_anything(x->x_obj.ob_outlet, x->x_sym, x->x_ac, x->x_at);
  }
  else
  {
    if(ac > x->x_size)
    {
      x->x_at = (t_atom *)resizebytes(x->x_at, x->x_size*sizeof(t_atom), ac*sizeof(t_atom));
      x->x_size = ac;
    }
    x->x_ac = ac;
    x->x_sym = s;
    iem_anything_kernel_atcopy(av, x->x_at, ac);
    if(!x->x_inlet_select)
      outlet_anything(x->x_obj.ob_outlet, s, ac, av);
  }
}

static void iem_anything_kernel_free(t_iem_anything_kernel *x)
{
  if(x->x_at)
    freebytes(x->x_at, x->x_size * sizeof(t_atom));
}

static void *iem_anything_kernel_new(void)
{
  t_iem_anything_kernel *x = (t_iem_anything_kernel *)pd_new(iem_anything_kernel_class);
  t_glist *glist = (t_glist *)canvas_getcurrent();
  t_canvas *canvas=glist_getcanvas(glist);
  int ac=0;
  t_atom *av;
  
  canvas_setcurrent(canvas);
  canvas_getargs(&ac, &av);
  canvas_unsetcurrent(canvas);
  
  if(!ac)
  {
    x->x_sym = &s_bang;
    x->x_size = 10;
    x->x_ac = 0;
    x->x_at = (t_atom *)getbytes(x->x_size * sizeof(t_atom));
  }
  else if(ac == 1)
  {
    x->x_size = 10;
    x->x_at = (t_atom *)getbytes(x->x_size * sizeof(t_atom));
    if(IS_A_SYMBOL(av,0))
    {
      x->x_sym = atom_getsymbol(av);
      x->x_ac = 0;
    }
    else
    {
      x->x_sym = &s_list;
      x->x_ac = 1;
      x->x_at[0] = *av;
    }
  }
  else /* ac > 1 */
  {
    if(IS_A_SYMBOL(av,0))
    {
      x->x_sym = atom_getsymbol(av++);
      ac--;
    }
    else
      x->x_sym = &s_list;
    if(ac < 10)
      x->x_size = 10;
    else
      x->x_size = ac;
    x->x_ac = ac;
    x->x_at = (t_atom *)getbytes(x->x_size * sizeof(t_atom));
    iem_anything_kernel_atcopy(av, x->x_at, ac);
  }
  x->x_inlet_select = 1;
  outlet_new(&x->x_obj, &s_list);
  return (x);
}

void iem_anything_kernel_setup(void)
{
  char str[2];
  
  str[0] = 1;/*inlet-sym = "\0x01"*/
  str[1] = 0;
  iem_anything_kernel_class = class_new(gensym("iem_anything_kernel"),
    (t_newmethod)iem_anything_kernel_new, (t_method)iem_anything_kernel_free,
    sizeof(t_iem_anything_kernel), 0, 0);
  class_addmethod(iem_anything_kernel_class, (t_method)iem_anything_kernel_inlet_select, gensym(str), A_FLOAT, 0);
  class_addanything(iem_anything_kernel_class, iem_anything_kernel_anything);
  class_sethelpsymbol(iem_anything_kernel_class, gensym("iemhelp/help-iem_anything"));
}
