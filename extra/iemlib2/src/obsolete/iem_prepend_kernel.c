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


/* ------------------------ iem_prepend_kernel ---------------------------- */
static t_class *iem_prepend_kernel_class;

typedef struct _iem_prepend_kernel
{
  t_object    x_obj;
  int         x_inlet_select;
  int         x_size;
  int         x_ac;
  t_atom      *x_at;
  t_symbol    *x_sym;
  t_atomtype  x_type;
} t_iem_prepend_kernel;

static void iem_prepend_kernel_atcopy(t_atom *src, t_atom *dst, int n)
{
  while(n--)
    *dst++ = *src++;
}

static void iem_prepend_kernel_inlet_select(t_iem_prepend_kernel *x, t_floatarg fin_sel_01)
{
  x->x_inlet_select = (int)fin_sel_01;
}

static void iem_prepend_kernel_bang(t_iem_prepend_kernel *x)
{
  if(x->x_inlet_select) /* if 2nd inlet */
  {
    x->x_ac = 0;
    x->x_type = A_NULL;
    x->x_sym = &s_bang;
  }
  else /* if 1st inlet */
  {
    outlet_anything(x->x_obj.ob_outlet, x->x_sym, x->x_ac, x->x_at);
  }
}

static void iem_prepend_kernel_float(t_iem_prepend_kernel *x, t_float f)
{
  if(x->x_inlet_select) /* if 2nd inlet */
  {
    x->x_ac = 1;
    x->x_type = A_FLOAT;
    SETFLOAT(x->x_at, f);
    x->x_sym = &s_list;
  }
  else /* if 1st inlet */
  {
    if(x->x_type == A_NULL)
    {
      outlet_float(x->x_obj.ob_outlet, f);
    }
    else
    {
      SETFLOAT(x->x_at+x->x_ac, f);
      outlet_anything(x->x_obj.ob_outlet, x->x_sym, x->x_ac+1, x->x_at);
    }
  }
}

static void iem_prepend_kernel_symbol(t_iem_prepend_kernel *x, t_symbol *s)
{
  if(x->x_inlet_select) /* if 2nd inlet */
  {
    x->x_ac = 1;
    x->x_type = A_SYMBOL;
    SETSYMBOL(x->x_at, s);
    x->x_sym = &s_list;
  }
  else /* if 1st inlet */
  {
    if(x->x_type == A_NULL)
    {
      outlet_symbol(x->x_obj.ob_outlet, s);
    }
    else
    {
      SETSYMBOL(x->x_at+x->x_ac, s);
      outlet_anything(x->x_obj.ob_outlet, x->x_sym, x->x_ac+1, x->x_at);
    }
  }
}

static void iem_prepend_kernel_pointer(t_iem_prepend_kernel *x, t_gpointer *gp)
{
  if(x->x_inlet_select) /* if 2nd inlet */
  {
    x->x_ac = 1;
    x->x_type = A_POINTER;
    SETPOINTER(x->x_at, gp);
    x->x_sym = &s_list;
  }
  else /* if 1st inlet */
  {
    if(x->x_type == A_NULL)
    {
      outlet_pointer(x->x_obj.ob_outlet, gp);
    }
    else
    {
      SETPOINTER(x->x_at+x->x_ac, gp);
      outlet_anything(x->x_obj.ob_outlet, x->x_sym, x->x_ac+1, x->x_at);
    }
  }
}

static void iem_prepend_kernel_list(t_iem_prepend_kernel *x, t_symbol *s, int ac, t_atom *av)
{
  if(x->x_inlet_select) /* if 2nd inlet */
  {
    if((ac+10) > x->x_size)
    {
      x->x_at = (t_atom *)resizebytes(x->x_at, x->x_size*sizeof(t_atom), 2*(ac+10)*sizeof(t_atom));
      x->x_size = 2*(ac+10);
    }
    x->x_ac = ac;
    x->x_type = A_GIMME;
    x->x_sym = &s_list;
    iem_prepend_kernel_atcopy(av, x->x_at, ac);
  }
  else /* if 1st inlet */
  {
    if((ac+x->x_ac+1) > x->x_size)
    {
      x->x_at = (t_atom *)resizebytes(x->x_at, x->x_size*sizeof(t_atom), 2*(ac+x->x_ac+1)*sizeof(t_atom));
      x->x_size = 2*(ac+x->x_ac+1);
    }
    if(x->x_type == A_NULL)
    {
      outlet_anything(x->x_obj.ob_outlet, &s_list, ac, av);
    }
    else
    {
      iem_prepend_kernel_atcopy(av, x->x_at + x->x_ac, ac);
      outlet_anything(x->x_obj.ob_outlet, x->x_sym, x->x_ac+ac, x->x_at);
    }
  }
}

static void iem_prepend_kernel_anything(t_iem_prepend_kernel *x, t_symbol *s, int ac, t_atom *av)
{
  if(x->x_inlet_select) /* if 2nd inlet */
  {
    if((ac+10) > x->x_size)
    {
      x->x_at = (t_atom *)resizebytes(x->x_at, x->x_size*sizeof(t_atom), 2*(ac+10)*sizeof(t_atom));
      x->x_size = 2*(ac+10);
    }
    x->x_ac = ac;
    x->x_type = A_COMMA;
    x->x_sym = s;
    iem_prepend_kernel_atcopy(av, x->x_at, ac);
  }
  else /* if 1st inlet */
  {
    if((ac+x->x_ac+1) > x->x_size)
    {
      x->x_at = (t_atom *)resizebytes(x->x_at, x->x_size*sizeof(t_atom), 2*(ac+x->x_ac+1)*sizeof(t_atom));
      x->x_size = 2*(ac+x->x_ac+1);
    }
    if(x->x_type == A_NULL)
    {
      outlet_anything(x->x_obj.ob_outlet, s, ac, av);
    }
    else
    {
      SETSYMBOL(x->x_at + x->x_ac, s);
      iem_prepend_kernel_atcopy(av, x->x_at+x->x_ac+1, ac);
      outlet_anything(x->x_obj.ob_outlet, x->x_sym, x->x_ac+ac+1, x->x_at);
    }
  }
}

static void iem_prepend_kernel_free(t_iem_prepend_kernel *x)
{
  if(x->x_at)
    freebytes(x->x_at, x->x_size * sizeof(t_atom));
}

static void *iem_prepend_kernel_new(void)
{
  t_iem_prepend_kernel *x = (t_iem_prepend_kernel *)pd_new(iem_prepend_kernel_class);
  t_glist *glist = (t_glist *)canvas_getcurrent();
  t_canvas *canvas=glist_getcanvas(glist);
  int ac=0;
  t_atom *av;
  
  canvas_setcurrent(canvas);
  canvas_getargs(&ac, &av);
  canvas_unsetcurrent(canvas);
  
  x->x_size = 30;
  if(ac > 5)
    x->x_size = 2*(ac+10);
  x->x_at = (t_atom *)getbytes(x->x_size * sizeof(t_atom));
  x->x_inlet_select = 1;
  if(ac <= 0)
  {
    x->x_type = A_NULL;
    x->x_ac = 0;
    x->x_sym = &s_bang;
  }
  else
  {
    if(IS_A_FLOAT(av, 0))
    {
      iem_prepend_kernel_list(x, &s_list, ac, av);
    }
    else if(IS_A_SYMBOL(av, 0))
    {
      iem_prepend_kernel_anything(x, atom_getsymbol(av), ac-1, av+1);
    }
  }
  outlet_new(&x->x_obj, &s_list);
  return(x);
}

void iem_prepend_kernel_setup(void)
{
  char str[2];
  
  str[0] = 1;
  str[1] = 0;
  iem_prepend_kernel_class = class_new(gensym("iem_prepend_kernel"),
    (t_newmethod)iem_prepend_kernel_new, (t_method)iem_prepend_kernel_free,
    sizeof(t_iem_prepend_kernel), 0, 0);
  class_addbang(iem_prepend_kernel_class, (t_method)iem_prepend_kernel_bang);
  class_addpointer(iem_prepend_kernel_class, iem_prepend_kernel_pointer);
  class_addfloat(iem_prepend_kernel_class, (t_method)iem_prepend_kernel_float);
  class_addsymbol(iem_prepend_kernel_class, iem_prepend_kernel_symbol);
  class_addlist(iem_prepend_kernel_class, iem_prepend_kernel_list);
  class_addmethod(iem_prepend_kernel_class, (t_method)iem_prepend_kernel_inlet_select, gensym(str), A_FLOAT, 0);
  class_addanything(iem_prepend_kernel_class, iem_prepend_kernel_anything);
  class_sethelpsymbol(iem_prepend_kernel_class, gensym("iemhelp/help-merge_any"));
}
