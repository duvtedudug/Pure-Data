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


/* ------------------------ iem_append_kernel ---------------------------- */
static t_class *iem_append_kernel_class;

typedef struct _iem_append_kernel
{
  t_object    x_obj;
  int         x_inlet_select;
  int         x_size12;
  int         x_size2;
  int         x_ac1;
  int         x_ac2;
  t_atom      *x_at12;
  t_atom      *x_at2;
  t_symbol    *x_sym1;
  t_symbol    *x_sym2;
  t_atomtype  x_type1;
  t_atomtype  x_type2;
} t_iem_append_kernel;

static void iem_append_kernel_atcopy(t_atom *src, t_atom *dst, int n)
{
  while(n--)
    *dst++ = *src++;
}

static void iem_append_kernel_merge(t_iem_append_kernel *x, int off)
{
  if((x->x_ac1+x->x_ac2+1) > x->x_size12)
  {
    x->x_at12 = (t_atom *)resizebytes(x->x_at12, x->x_size12*sizeof(t_atom), 2*(x->x_ac1+x->x_ac2+1)*sizeof(t_atom));
    x->x_size12 = 2*(x->x_ac1+x->x_ac2+1);
  }
  if(off)
    SETSYMBOL(x->x_at12 + x->x_ac1, x->x_sym2);
  iem_append_kernel_atcopy(x->x_at2, x->x_at12 + x->x_ac1 + off, x->x_ac2);
}

static void iem_append_kernel_out(t_iem_append_kernel *x)
{
  int off=0;
  
  if(x->x_type1 == A_GIMME)
  {
    if(x->x_type2 == A_COMMA)
      off = 1;
    else
      off = 0;
    iem_append_kernel_merge(x, off);
    outlet_list(x->x_obj.ob_outlet, &s_list, x->x_ac1+x->x_ac2+off, x->x_at12);
  }
  else if(x->x_type1 == A_COMMA)
  {
    if(x->x_type2 == A_COMMA)
      off = 1;
    else
      off = 0;
    iem_append_kernel_merge(x, off);
    outlet_anything(x->x_obj.ob_outlet, x->x_sym1, x->x_ac1+x->x_ac2+off, x->x_at12);
  }
  else if(x->x_type1 == A_NULL)/*depends on 2.part*/
  {
    iem_append_kernel_merge(x, 0);
    if(x->x_type2 == A_GIMME)
      outlet_list(x->x_obj.ob_outlet, &s_list, x->x_ac2, x->x_at12);
    else if(x->x_type2 == A_COMMA)
      outlet_anything(x->x_obj.ob_outlet, x->x_sym2, x->x_ac2, x->x_at12);
    else if(x->x_type2 == A_FLOAT)
      outlet_float(x->x_obj.ob_outlet, atom_getfloat(x->x_at12));
    else if(x->x_type2 == A_SYMBOL)
      outlet_symbol(x->x_obj.ob_outlet, atom_getsymbol(x->x_at12));
    else if(x->x_type2 == A_NULL)
      outlet_bang(x->x_obj.ob_outlet);
    else if(x->x_type2 == A_POINTER)
      outlet_pointer(x->x_obj.ob_outlet, (t_gpointer *)x->x_at12->a_w.w_gpointer);
  }
  else
  {
    if(x->x_type2 == A_COMMA)
      off = 1;
    else
      off = 0;
    iem_append_kernel_merge(x, off);
    if(x->x_type2 == A_NULL)
    {
      if(x->x_type1 == A_FLOAT)
        outlet_float(x->x_obj.ob_outlet, atom_getfloat(x->x_at12));
      else if(x->x_type1 == A_SYMBOL)
        outlet_symbol(x->x_obj.ob_outlet, atom_getsymbol(x->x_at12));
      else if(x->x_type1 == A_POINTER)
        outlet_pointer(x->x_obj.ob_outlet, (t_gpointer *)x->x_at12->a_w.w_gpointer);
    }
    else
      outlet_list(x->x_obj.ob_outlet, &s_list, x->x_ac1+x->x_ac2+off, x->x_at12);
  }
}

static void iem_append_kernel_inlet_select(t_iem_append_kernel *x, t_floatarg fin_sel_01)
{
  x->x_inlet_select = (int)fin_sel_01;
}

static void iem_append_kernel_bang(t_iem_append_kernel *x)
{
  if(x->x_inlet_select) /* if 2nd inlet */
  {
    x->x_ac2 = 0;
    x->x_type2 = A_NULL;
    x->x_sym2 = &s_list;
  }
  else /* if 1st inlet */
  {
    x->x_ac1 = 0;
    x->x_type1 = A_NULL;
    iem_append_kernel_out(x);
  }
}

static void iem_append_kernel_float(t_iem_append_kernel *x, t_float f)
{
  if(x->x_inlet_select) /* if 2nd inlet */
  {
    x->x_ac2 = 1;
    x->x_type2 = A_FLOAT;
    SETFLOAT(x->x_at2, f);
    x->x_sym2 = &s_list;
  }
  else /* if 1st inlet */
  {
    x->x_ac1 = 1;
    x->x_type1 = A_FLOAT;
    SETFLOAT(x->x_at12, f);
    iem_append_kernel_out(x);
  }
}

static void iem_append_kernel_symbol(t_iem_append_kernel *x, t_symbol *s)
{
  if(x->x_inlet_select) /* if 2nd inlet */
  {
    x->x_ac2 = 1;
    x->x_type2 = A_SYMBOL;
    SETSYMBOL(x->x_at2, s);
    x->x_sym2 = &s_list;
  }
  else /* if 1st inlet */
  {
    x->x_ac1 = 1;
    x->x_type1 = A_SYMBOL;
    SETSYMBOL(x->x_at12, s);
    iem_append_kernel_out(x);
  }
}

static void iem_append_kernel_pointer(t_iem_append_kernel *x, t_gpointer *gp)
{
  if(x->x_inlet_select) /* if 2nd inlet */
  {
    x->x_ac2 = 1;
    x->x_type2 = A_POINTER;
    SETPOINTER(x->x_at2, gp);
    x->x_sym2 = &s_list;
  }
  else /* if 1st inlet */
  {
    x->x_ac1 = 1;
    x->x_type1 = A_POINTER;
    SETPOINTER(x->x_at12, gp);
    iem_append_kernel_out(x);
  }
}

static void iem_append_kernel_list(t_iem_append_kernel *x, t_symbol *s, int ac, t_atom *av)
{
  if(x->x_inlet_select) /* if 2nd inlet */
  {
    if(ac > x->x_size2)
    {
      x->x_at2 = (t_atom *)resizebytes(x->x_at2, x->x_size2*sizeof(t_atom), 2*ac*sizeof(t_atom));
      x->x_size2 = 2*ac;
    }
    x->x_ac2 = ac;
    x->x_type2 = A_GIMME;
    x->x_sym2 = &s_list;
    iem_append_kernel_atcopy(av, x->x_at2, ac);
  }
  else /* if 1st inlet */
  {
    if((x->x_size2+ac) > x->x_size12)
    {
      x->x_at12 = (t_atom *)resizebytes(x->x_at12, x->x_size12*sizeof(t_atom), 2*(x->x_size2+ac)*sizeof(t_atom));
      x->x_size12 = 2*(x->x_size2+ac);
    }
    x->x_ac1 = ac;
    x->x_type1 = A_GIMME;
    iem_append_kernel_atcopy(av, x->x_at12, ac);
    x->x_sym1 = &s_list;
    iem_append_kernel_out(x);
  }
}

static void iem_append_kernel_anything(t_iem_append_kernel *x, t_symbol *s, int ac, t_atom *av)
{
  if(x->x_inlet_select) /* if 2nd inlet */
  {
    if((ac+1) > x->x_size2)
    {
      x->x_at2 = (t_atom *)resizebytes(x->x_at2, x->x_size2*sizeof(t_atom), 2*ac*sizeof(t_atom));
      x->x_size2 = 2*ac;
    }
    x->x_ac2 = ac;
    x->x_type2 = A_COMMA;
    x->x_sym2 = s;
    iem_append_kernel_atcopy(av, x->x_at2, ac);
  }
  else /* if 1st inlet */
  {
    if((x->x_size2+ac+1) > x->x_size12)
    {
      x->x_at12 = (t_atom *)resizebytes(x->x_at12, x->x_size12*sizeof(t_atom), 2*(x->x_size2+ac+1)*sizeof(t_atom));
      x->x_size12 = 2*(x->x_size2+ac+1);
    }
    x->x_ac1 = ac;
    x->x_type1 = A_COMMA;
    iem_append_kernel_atcopy(av, x->x_at12, ac);
    x->x_sym1 = s;
    iem_append_kernel_out(x);
  }
}

static void iem_append_kernel_free(t_iem_append_kernel *x)
{
  if(x->x_at12)
    freebytes(x->x_at12, x->x_size12 * sizeof(t_atom));
  if(x->x_at2)
    freebytes(x->x_at2, x->x_size2 * sizeof(t_atom));
}

static void *iem_append_kernel_new(void)
{
  t_iem_append_kernel *x = (t_iem_append_kernel *)pd_new(iem_append_kernel_class);
  t_glist *glist = (t_glist *)canvas_getcurrent();
  t_canvas *canvas=glist_getcanvas(glist);
  int ac=0;
  t_atom *av;
  
  canvas_setcurrent(canvas);
  canvas_getargs(&ac, &av);
  canvas_unsetcurrent(canvas);
  
  x->x_type1 = A_NULL;
  x->x_sym1 = &s_list;
  x->x_size2 = 10;
  if(ac > 5)
    x->x_size2 = 2*ac;
  x->x_at2 = (t_atom *)getbytes(x->x_size2 * sizeof(t_atom));
  x->x_size12 = x->x_size2 + 10;
  x->x_at12 = (t_atom *)getbytes(x->x_size12 * sizeof(t_atom));
  x->x_ac1 = 0;
  x->x_inlet_select = 1;
  if(ac <= 0)
  {
    x->x_type2 = A_NULL;
    x->x_ac2 = 0;
    x->x_sym2 = &s_list;
  }
  else
  {
    if(IS_A_FLOAT(av, 0))
    {
      if(ac == 1)
        iem_append_kernel_float(x, atom_getfloat(av));
      else
        iem_append_kernel_list(x, &s_list, ac, av);
    }
    else if(IS_A_SYMBOL(av, 0))
    {
      t_symbol *xsym=atom_getsymbol(av);
      
      if(xsym == gensym("symbol"))
      {
        if(ac > 1)
          iem_append_kernel_symbol(x, atom_getsymbol(av+1));
        else
          iem_append_kernel_symbol(x, gensym(""));
      }
      else if(xsym == gensym("float"))
      {
        if(ac > 1)
        {
          if(IS_A_FLOAT(av, 1))
            iem_append_kernel_float(x, atom_getfloat(av+1));
          else
            iem_append_kernel_float(x, 0.0f);
        }
        else
          iem_append_kernel_float(x, 0.0f);
      }
      else if(xsym == gensym("list"))
      {
        iem_append_kernel_list(x, &s_list, ac-1, av+1);
      }
      else
      {
        iem_append_kernel_anything(x, xsym, ac-1, av+1);
      }
    }
  }
  outlet_new(&x->x_obj, &s_list);
  return (x);
}

void iem_append_kernel_setup(void)
{
  char str[2];
  
  str[0] = 1;
  str[1] = 0;
  iem_append_kernel_class = class_new(gensym("iem_append_kernel"),
    (t_newmethod)iem_append_kernel_new, (t_method)iem_append_kernel_free,
    sizeof(t_iem_append_kernel), 0, 0);
  class_addbang(iem_append_kernel_class, (t_method)iem_append_kernel_bang);
  class_addpointer(iem_append_kernel_class, iem_append_kernel_pointer);
  class_addfloat(iem_append_kernel_class, (t_method)iem_append_kernel_float);
  class_addsymbol(iem_append_kernel_class, iem_append_kernel_symbol);
  class_addlist(iem_append_kernel_class, iem_append_kernel_list);
  class_addmethod(iem_append_kernel_class, (t_method)iem_append_kernel_inlet_select, gensym(str), A_FLOAT, 0);
  class_addanything(iem_append_kernel_class, iem_append_kernel_anything);
  class_sethelpsymbol(iem_append_kernel_class, gensym("iemhelp/help-merge_any"));
}
