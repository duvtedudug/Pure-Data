/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib1 written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2006 */


#include "m_pd.h"
#include "iemlib.h"
#include <math.h>


/* ---------- filter~ - slow dynamic filter-kernel 1. and 2. order ----------- */

typedef struct _filter_tilde
{
  t_object  x_obj;
  t_float   wn1;
  t_float   wn2;
  t_float   a0;
  t_float   a1;
  t_float   a2;
  t_float   b1;
  t_float   b2;
  t_float   sr;
  t_float   cur_f;
  t_float   cur_l;
  t_float   cur_a;
  t_float   cur_b;
  t_float   delta_f;
  t_float   delta_a;
  t_float   delta_b;
  t_float   end_f;
  t_float   end_a;
  t_float   end_b;
  t_float   ticks_per_interpol_time;
  t_float   rcp_ticks;
  t_float   interpol_time;
  int       ticks;
  int       counter_f;
  int       counter_a;
  int       counter_b;
  int       inv;
  int       hp;
  int       first_order;
  int       event_mask;
  void      (*calc)();
  void      *x_debug_outlet;
  t_atom    x_at[5];
  t_float   x_msi;
} t_filter_tilde;

t_class *filter_tilde_class;

static void filter_tilde_snafu(t_filter_tilde *x)
{
  
}

static void filter_tilde_lp1(t_filter_tilde *x)
{
  t_float al;
  
  al = x->cur_a * x->cur_l;
  x->a0 = 1.0f/(1.0f + al);
  x->a1 = x->a0;
  x->b1 = (al - 1.0f)*x->a0;
}

static void filter_tilde_lp2(t_filter_tilde *x)
{
  t_float l, al, bl2, rcp;
  
  l = x->cur_l;
  al = l*x->cur_a;
  bl2 = l*l*x->cur_b + 1.0f;
  rcp = 1.0f/(al + bl2);
  x->a0 = rcp;
  x->a1 = 2.0f*rcp;
  x->a2 = x->a0;
  x->b1 = rcp*2.0f*(bl2 - 2.0f);
  x->b2 = rcp*(al - bl2);
}

static void filter_tilde_hp1(t_filter_tilde *x)
{
  t_float al, rcp;
  
  al = x->cur_a * x->cur_l;
  rcp = 1.0f/(1.0f + al);
  x->a0 = rcp*al;
  x->a1 = -x->a0;
  x->b1 = rcp*(al - 1.0f);
}

static void filter_tilde_hp2(t_filter_tilde *x)
{
  t_float l, al, bl2, rcp;
  
  l = x->cur_l;
  bl2 = l*l*x->cur_b + 1.0f;
  al = l*x->cur_a;
  rcp = 1.0f/(al + bl2);
  x->a0 = rcp*(bl2 - 1.0f);
  x->a1 = -2.0f*x->a0;
  x->a2 = x->a0;
  x->b1 = rcp*2.0f*(bl2 - 2.0f);
  x->b2 = rcp*(al - bl2);
}

static void filter_tilde_rp2(t_filter_tilde *x)
{
  t_float l, al, l2, rcp;
  
  l = x->cur_l;
  l2 = l*l + 1.0f;
  al = l*x->cur_a;
  rcp = 1.0f/(al + l2);
  x->a0 = rcp*l;
  x->a2 = -x->a0;
  x->b1 = rcp*2.0f*(l2 - 2.0f);
  x->b2 = rcp*(al - l2);
}

static void filter_tilde_bp2(t_filter_tilde *x)
{
  t_float l, al, l2, rcp;
  
  l = x->cur_l;
  l2 = l*l + 1.0f;
  al = l*x->cur_a;
  rcp = 1.0f/(al + l2);
  x->a0 = rcp*al;
  x->a2 = -x->a0;
  x->b1 = rcp*2.0f*(l2 - 2.0f);
  x->b2 = rcp*(al - l2);
}

static void filter_tilde_bs2(t_filter_tilde *x)
{
  t_float l, al, l2, rcp;
  
  l = x->cur_l;
  l2 = l*l + 1.0f;
  al = l*x->cur_a;
  rcp = 1.0f/(al + l2);
  x->a0 = rcp*l2;
  x->a1 = rcp*2.0f*(2.0f - l2);
  x->a2 = x->a0;
  x->b1 = -x->a1;
  x->b2 = rcp*(al - l2);
}

static void filter_tilde_rpw2(t_filter_tilde *x)
{
  t_float l, al, l2, rcp;
  
  l = x->cur_l;
  l2 = l*l + 1.0f;
  al = l*x->cur_a/x->cur_f;
  rcp = 1.0f/(al + l2);
  x->a0 = rcp*l;
  x->a2 = -x->a0;
  x->b1 = rcp*2.0f*(l2 - 2.0f);
  x->b2 = rcp*(al - l2);
}

static void filter_tilde_bpw2(t_filter_tilde *x)
{
  t_float l, al, l2, rcp;
  
  l = x->cur_l;
  l2 = l*l + 1.0f;
  al = l*x->cur_a/x->cur_f;
  rcp = 1.0f/(al + l2);
  x->a0 = rcp*al;
  x->a2 = -x->a0;
  x->b1 = rcp*2.0f*(l2 - 2.0f);
  x->b2 = rcp*(al - l2);
}

static void filter_tilde_bsw2(t_filter_tilde *x)
{
  t_float l, al, l2, rcp;
  
  l = x->cur_l;
  l2 = l*l + 1.0f;
  al = l*x->cur_a/x->cur_f;
  rcp = 1.0f/(al + l2);
  x->a0 = rcp*l2;
  x->a1 = rcp*2.0f*(2.0f - l2);
  x->a2 = x->a0;
  x->b1 = -x->a1;
  x->b2 = rcp*(al - l2);
}

static void filter_tilde_ap1(t_filter_tilde *x)
{
  t_float al;
  
  al = x->cur_a * x->cur_l;
  x->a0 = (1.0f - al)/(1.0f + al);
  x->b1 = -x->a0;
}

static void filter_tilde_ap2(t_filter_tilde *x)
{
  t_float l, al, bl2, rcp;
  
  l = x->cur_l;
  bl2 = l*l*x->cur_b + 1.0f;
  al = l*x->cur_a;
  rcp = 1.0f/(al + bl2);
  x->a1 = rcp*2.0f*(2.0f - bl2);
  x->a0 = rcp*(bl2 - al);
  x->b1 = -x->a1;
  x->b2 = -x->a0;
}

/*static void filter_tilde_bp2(t_filter_tilde *x)
{
t_float l, al, l2, rcp;

  l = x->cur_l;
  l2 = l*l + 1.0;
  al = l*x->cur_a;
  rcp = 1.0f/(al + l2);
  x->a0 = rcp*al;
  x->a2 = -x->a0;
  x->b1 = rcp*2.0f*(2.0f - l2);
  x->b2 = rcp*(l2 - al);
}*/

static void filter_tilde_dsp_tick(t_filter_tilde *x)
{
  if(x->event_mask)
  {
    if(x->counter_f)
    {
      float l, si, co;
      
      if(x->counter_f <= 1)
      {
        x->cur_f = x->end_f;
        x->counter_f = 0;
        x->event_mask &= 6;/*set event_mask_bit 0 = 0*/
      }
      else
      {
        x->counter_f--;
        x->cur_f *= x->delta_f;
      }
      l = x->cur_f * x->sr;
      if(l < 1.0e-20f)
        x->cur_l = 1.0e20f;
      else if(l > 1.57079632f)
        x->cur_l = 0.0f;
      else
      {
        si = sin(l);
        co = cos(l);
        x->cur_l = co/si;
      }
    }
    if(x->counter_a)
    {
      if(x->counter_a <= 1)
      {
        x->cur_a = x->end_a;
        x->counter_a = 0;
        x->event_mask &= 5;/*set event_mask_bit 1 = 0*/
      }
      else
      {
        x->counter_a--;
        x->cur_a *= x->delta_a;
      }
    }
    if(x->counter_b)
    {
      if(x->counter_b <= 1)
      {
        x->cur_b = x->end_b;
        x->counter_b = 0;
        x->event_mask &= 3;/*set event_mask_bit 2 = 0*/
      }
      else
      {
        x->counter_b--;
        x->cur_b *= x->delta_b;
      }
    }
    
    (*(x->calc))(x);
    
    /* stability check */
    if(x->first_order)
    {
      if(x->b1 <= -0.9999998f)
        x->b1 = -0.9999998f;
      else if(x->b1 >= 0.9999998f)
        x->b1 = 0.9999998f;
    }
    else
    {
      float discriminant = x->b1 * x->b1 + 4.0f * x->b2;
      
      if(x->b1 <= -1.9999996f)
        x->b1 = -1.9999996f;
      else if(x->b1 >= 1.9999996f)
        x->b1 = 1.9999996f;
      
      if(x->b2 <= -0.9999998f)
        x->b2 = -0.9999998f;
      else if(x->b2 >= 0.9999998f)
        x->b2 = 0.9999998f;
      
      if(discriminant >= 0.0f)
      {
        if(0.9999998f - x->b1 - x->b2 < 0.0f)
          x->b2 = 0.9999998f - x->b1;
        if(0.9999998f + x->b1 - x->b2 < 0.0f)
          x->b2 = 0.9999998f + x->b1;
      }
    }
  }
}

static t_int *filter_tilde_perform_2o(t_int *w)
{
  t_float *in = (float *)(w[1]);
  t_float *out = (float *)(w[2]);
  t_filter_tilde *x = (t_filter_tilde *)(w[3]);
  int i, n = (t_int)(w[4]);
  t_float wn0, wn1=x->wn1, wn2=x->wn2;
  t_float a0=x->a0, a1=x->a1, a2=x->a2;
  t_float b1=x->b1, b2=x->b2;
  
  filter_tilde_dsp_tick(x);
  for(i=0; i<n; i++)
  {
    wn0 = *in++ + b1*wn1 + b2*wn2;
    *out++ = a0*wn0 + a1*wn1 + a2*wn2;
    wn2 = wn1;
    wn1 = wn0;
  }
  /* NAN protect */
  if(IEM_DENORMAL(wn2))
    wn2 = 0.0f;
  if(IEM_DENORMAL(wn1))
    wn1 = 0.0f;
  
  x->wn1 = wn1;
  x->wn2 = wn2;
  return(w+5);
}
/*   yn0 = *out;
xn0 = *in;
*************
yn0 = a0*xn0 + a1*xn1 + a2*xn2 + b1*yn1 + b2*yn2;
yn2 = yn1;
yn1 = yn0;
xn2 = xn1;
xn1 = xn0;
*************************
y/x = (a0 + a1*z-1 + a2*z-2)/(1 - b1*z-1 - b2*z-2);*/

static t_int *filter_tilde_perf8_2o(t_int *w)
{
  t_float *in = (float *)(w[1]);
  t_float *out = (float *)(w[2]);
  t_filter_tilde *x = (t_filter_tilde *)(w[3]);
  int i, n = (t_int)(w[4]);
  t_float wn[10];
  t_float a0=x->a0, a1=x->a1, a2=x->a2;
  t_float b1=x->b1, b2=x->b2;
  
  filter_tilde_dsp_tick(x);
  wn[0] = x->wn2;
  wn[1] = x->wn1;
  for(i=0; i<n; i+=8, in+=8, out+=8)
  {
    wn[2] = in[0] + b1*wn[1] + b2*wn[0];
    out[0] = a0*wn[2] + a1*wn[1] + a2*wn[0];
    wn[3] = in[1] + b1*wn[2] + b2*wn[1];
    out[1] = a0*wn[3] + a1*wn[2] + a2*wn[1];
    wn[4] = in[2] + b1*wn[3] + b2*wn[2];
    out[2] = a0*wn[4] + a1*wn[3] + a2*wn[2];
    wn[5] = in[3] + b1*wn[4] + b2*wn[3];
    out[3] = a0*wn[5] + a1*wn[4] + a2*wn[3];
    wn[6] = in[4] + b1*wn[5] + b2*wn[4];
    out[4] = a0*wn[6] + a1*wn[5] + a2*wn[4];
    wn[7] = in[5] + b1*wn[6] + b2*wn[5];
    out[5] = a0*wn[7] + a1*wn[6] + a2*wn[5];
    wn[8] = in[6] + b1*wn[7] + b2*wn[6];
    out[6] = a0*wn[8] + a1*wn[7] + a2*wn[6];
    wn[9] = in[7] + b1*wn[8] + b2*wn[7];
    out[7] = a0*wn[9] + a1*wn[8] + a2*wn[7];
    wn[0] = wn[8];
    wn[1] = wn[9];
  }
  /* NAN protect */
  if(IEM_DENORMAL(wn[0]))
    wn[0] = 0.0f;
  if(IEM_DENORMAL(wn[1]))
    wn[1] = 0.0f;
  
  x->wn1 = wn[1];
  x->wn2 = wn[0];
  return(w+5);
}

static t_int *filter_tilde_perform_1o(t_int *w)
{
  t_float *in = (float *)(w[1]);
  t_float *out = (float *)(w[2]);
  t_filter_tilde *x = (t_filter_tilde *)(w[3]);
  int i, n = (t_int)(w[4]);
  t_float wn0, wn1=x->wn1;
  t_float a0=x->a0, a1=x->a1;
  t_float b1=x->b1;
  
  filter_tilde_dsp_tick(x);
  for(i=0; i<n; i++)
  {
    wn0 = *in++ + b1*wn1;
    *out++ = a0*wn0 + a1*wn1;
    wn1 = wn0;
  }
  /* NAN protect */
  if(IEM_DENORMAL(wn1))
    wn1 = 0.0f;
  
  x->wn1 = wn1;
  return(w+5);
}

static t_int *filter_tilde_perf8_1o(t_int *w)
{
  t_float *in = (float *)(w[1]);
  t_float *out = (float *)(w[2]);
  t_filter_tilde *x = (t_filter_tilde *)(w[3]);
  int i, n = (t_int)(w[4]);
  t_float wn[9];
  t_float a0=x->a0, a1=x->a1;
  t_float b1=x->b1;
  
  filter_tilde_dsp_tick(x);
  wn[0] = x->wn1;
  for(i=0; i<n; i+=8, in+=8, out+=8)
  {
    wn[1] = in[0] + b1*wn[0];
    out[0] = a0*wn[1] + a1*wn[0];
    wn[2] = in[1] + b1*wn[1];
    out[1] = a0*wn[2] + a1*wn[1];
    wn[3] = in[2] + b1*wn[2];
    out[2] = a0*wn[3] + a1*wn[2];
    wn[4] = in[3] + b1*wn[3];
    out[3] = a0*wn[4] + a1*wn[3];
    wn[5] = in[4] + b1*wn[4];
    out[4] = a0*wn[5] + a1*wn[4];
    wn[6] = in[5] + b1*wn[5];
    out[5] = a0*wn[6] + a1*wn[5];
    wn[7] = in[6] + b1*wn[6];
    out[6] = a0*wn[7] + a1*wn[6];
    wn[8] = in[7] + b1*wn[7];
    out[7] = a0*wn[8] + a1*wn[7];
    wn[0] = wn[8];
  }
  /* NAN protect */
  if(IEM_DENORMAL(wn[0]))
    wn[0] = 0.0f;
  
  x->wn1 = wn[0];
  return(w+5);
}

static void filter_tilde_ft4(t_filter_tilde *x, t_floatarg t)
{
  int i = (int)((x->ticks_per_interpol_time)*t+0.49999f);
  
  x->interpol_time = t;
  if(i <= 0)
  {
    x->ticks = 1;
    x->rcp_ticks = 1.0;
  }
  else
  {
    x->ticks = i;
    x->rcp_ticks = 1.0 / (t_float)i;
  }
}

static void filter_tilde_ft3(t_filter_tilde *x, t_floatarg b)
{
  if(b <= 0.0f)
    b = 0.000001f;
  if(x->hp)
    b = 1.0 / b;
  if(b != x->cur_b)
  {
    x->end_b = b;
    x->counter_b = x->ticks;
    x->delta_b = exp(log(b/x->cur_b)*x->rcp_ticks);
    x->event_mask |= 4;/*set event_mask_bit 2 = 1*/
  }
}

static void filter_tilde_ft2(t_filter_tilde *x, t_floatarg a)
{
  if(a <= 0.0f)
    a = 0.000001f;
  if(x->inv)
    a = 1.0f / a;
  if(x->hp)
    a /= x->cur_b;
  if(a != x->cur_a)
  {
    x->end_a = a;
    x->counter_a = x->ticks;
    x->delta_a = exp(log(a/x->cur_a)*x->rcp_ticks);
    x->event_mask |= 2;/*set event_mask_bit 1 = 1*/
  }
}

static void filter_tilde_ft1(t_filter_tilde *x, t_floatarg f)
{
  if(f <= 0.0f)
    f = 0.000001f;
  if(f != x->cur_f)
  {
    x->end_f = f;
    x->counter_f = x->ticks;
    x->delta_f = exp(log(f/x->cur_f)*x->rcp_ticks);
    x->event_mask |= 1;/*set event_mask_bit 0 = 1*/
  }
}

static void filter_tilde_print(t_filter_tilde *x)
{
  //  post("fb1 = %g, fb2 = %g, ff1 = %g, ff2 = %g, ff3 = %g", x->b1, x->b2, x->a0, x->a1, x->a2);
  x->x_at[0].a_w.w_float = x->b1;
  x->x_at[1].a_w.w_float = x->b2;
  x->x_at[2].a_w.w_float = x->a0;
  x->x_at[3].a_w.w_float = x->a1;
  x->x_at[4].a_w.w_float = x->a2;
  outlet_list(x->x_debug_outlet, &s_list, 5, x->x_at);
}

static void filter_tilde_dsp(t_filter_tilde *x, t_signal **sp)
{
  t_float si, co, f;
  int i, n=(int)sp[0]->s_n;
  
  x->sr = 3.14159265358979323846f / (t_float)(sp[0]->s_sr);
  x->ticks_per_interpol_time = 0.001f * (t_float)(sp[0]->s_sr) / (t_float)n;
  i = (int)((x->ticks_per_interpol_time)*(x->interpol_time)+0.49999f);
  if(i <= 0)
  {
    x->ticks = 1;
    x->rcp_ticks = 1.0f;
  }
  else
  {
    x->ticks = i;
    x->rcp_ticks = 1.0f / (t_float)i;
  }
  f = x->cur_f * x->sr;
  if(f < 1.0e-20f)
    x->cur_l = 1.0e20f;
  else if(f > 1.57079632f)
    x->cur_l = 0.0f;
  else
  {
    si = sin(f);
    co = cos(f);
    x->cur_l = co/si;
  }
  if(x->first_order)
  {
    if(n&7)
      dsp_add(filter_tilde_perform_1o, 4, sp[0]->s_vec, sp[1]->s_vec, x, n);
    else
      dsp_add(filter_tilde_perf8_1o, 4, sp[0]->s_vec, sp[1]->s_vec, x, n);
  }
  else
  {
    if(n&7)
      dsp_add(filter_tilde_perform_2o, 4, sp[0]->s_vec, sp[1]->s_vec, x, n);
    else
      dsp_add(filter_tilde_perf8_2o, 4, sp[0]->s_vec, sp[1]->s_vec, x, n);
  }
}

static void *filter_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
  t_filter_tilde *x = (t_filter_tilde *)pd_new(filter_tilde_class);
  int i;
  t_float si, co, f=0.0f, a=0.0f, b=0.0f, interpol=0.0f;
  t_symbol *filt_typ=gensym("");
  
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft2"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft3"));
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft4"));
  outlet_new(&x->x_obj, &s_signal);
  x->x_debug_outlet = outlet_new(&x->x_obj, &s_list);
  x->x_msi = 0.0f;
  
  x->x_at[0].a_type = A_FLOAT;
  x->x_at[1].a_type = A_FLOAT;
  x->x_at[2].a_type = A_FLOAT;
  x->x_at[3].a_type = A_FLOAT;
  x->x_at[4].a_type = A_FLOAT;
  
  x->event_mask = 1;
  x->counter_f = 1;
  x->counter_a = 0;
  x->counter_b = 0;
  x->delta_f = 0.0f;
  x->delta_a = 0.0f;
  x->delta_b = 0.0f;
  x->interpol_time = 0.0f;
  x->wn1 = 0.0f;
  x->wn2 = 0.0f;
  x->a0 = 0.0f;
  x->a1 = 0.0f;
  x->a2 = 0.0f;
  x->b1 = 0.0f;
  x->b2 = 0.0f;
  x->sr = 3.14159265358979323846f / 44100.0f;
  x->calc = filter_tilde_snafu;
  x->first_order = 0;
  if((argc == 5)&&IS_A_FLOAT(argv,4)&&IS_A_FLOAT(argv,3)&&IS_A_FLOAT(argv,2)&&IS_A_FLOAT(argv,1)&&IS_A_SYMBOL(argv,0))
  {
    filt_typ = atom_getsymbolarg(0, argc, argv);
    f = (t_float)atom_getfloatarg(1, argc, argv);
    a = (t_float)atom_getfloatarg(2, argc, argv);
    b = (t_float)atom_getfloatarg(3, argc, argv);
    interpol = (t_float)atom_getfloatarg(4, argc, argv);
  }
  x->cur_f = f;
  f *= x->sr;
  if(f < 1.0e-20f)
    x->cur_l = 1.0e20f;
  else if(f > 1.57079632f)
    x->cur_l = 0.0f;
  else
  {
    si = sin(f);
    co = cos(f);
    x->cur_l = co/si;
  }
  if(a <= 0.0f)
    a = 0.000001f;
  if(b <= 0.0f)
    b = 0.000001f;
  x->cur_b = b;
  
  if(interpol <= 0.0f)
    interpol = 0.0f;
  x->interpol_time = interpol;
  x->ticks_per_interpol_time = 0.001f * 44100.0f / 64.0f;
  i = (int)((x->ticks_per_interpol_time)*(x->interpol_time)+0.49999f);
  if(i <= 0)
  {
    x->ticks = 1;
    x->rcp_ticks = 1.0f;
  }
  else
  {
    x->ticks = i;
    x->rcp_ticks = 1.0f / (t_float)i;
  }
  
  x->calc = filter_tilde_snafu;
  
  x->cur_a = 1.0f/a; /*a was Q*/
  x->inv = 1;
  x->hp = 0;
  
  if(filt_typ->s_name)
  {
    if(filt_typ == gensym("ap1"))
    {
      x->calc = filter_tilde_ap1;
      x->a1 = 1.0f;
      x->first_order = 1;
    }
    else if(filt_typ == gensym("ap2"))
    {
      x->calc = filter_tilde_ap2;
      x->a2 = 1.0f;
    }
    else if(filt_typ == gensym("ap1c"))
    {
      x->calc = filter_tilde_ap1;
      x->a1 = 1.0f;
      x->inv = 0;
      x->cur_a = a; /*a was damping*/
      x->first_order = 1;
    }
    else if(filt_typ == gensym("ap2c"))
    {
      x->calc = filter_tilde_ap2;
      x->a2 = 1.0f;
      x->inv = 0;
      x->cur_a = a; /*a was damping*/
    }
    else if(filt_typ == gensym("bpq2"))
    {
      x->calc = filter_tilde_bp2;
    }
    else if(filt_typ == gensym("rbpq2"))
    {
      x->calc = filter_tilde_rp2;
    }
    else if(filt_typ == gensym("bsq2"))
    {
      x->calc = filter_tilde_bs2;
    }
    else if(filt_typ == gensym("bpw2"))
    {
      x->calc = filter_tilde_bpw2;
      x->inv = 0;
      x->cur_a = a; /*a was bw*/
    }
    else if(filt_typ == gensym("rbpw2"))
    {
      x->calc = filter_tilde_rpw2;
      x->inv = 0;
      x->cur_a = a; /*a was bw*/
    }
    else if(filt_typ == gensym("bsw2"))
    {
      x->calc = filter_tilde_bsw2;
      x->inv = 0;
      x->cur_a = a; /*a was bw*/
    }
    else if(filt_typ == gensym("hp1"))
    {
      x->calc = filter_tilde_hp1;
      x->first_order = 1;
    }
    else if(filt_typ == gensym("hp2"))
    {
      x->calc = filter_tilde_hp2;
    }
    else if(filt_typ == gensym("lp1"))
    {
      x->calc = filter_tilde_lp1;
      x->first_order = 1;
    }
    else if(filt_typ == gensym("lp2"))
    {
      x->calc = filter_tilde_lp2;
    }
    else if(filt_typ == gensym("hp1c"))
    {
      x->calc = filter_tilde_hp1;
      x->cur_a = 1.0f / a;
      x->first_order = 1;
    }
    else if(filt_typ == gensym("hp2c"))
    {
      x->calc = filter_tilde_hp2;
      x->inv = 0;
      x->cur_a = a / b;
      x->cur_b = 1.0f / b;
      x->hp = 1;
    }
    else if(filt_typ == gensym("lp1c"))
    {
      x->calc = filter_tilde_lp1;
      x->inv = 0;
      x->cur_a = a; /*a was damping*/
      x->first_order = 1;
    }
    else if(filt_typ == gensym("lp2c"))
    {
      x->calc = filter_tilde_lp2;
      x->inv = 0;
      x->cur_a = a; /*a was damping*/
    }
    else
    {
      post("filter~-Error: 1. initial-arguments: <sym> kind: \
lp1, lp2, hp1, hp2, \
lp1c, lp2c, hp1c, hp2c, \
ap1, ap2, ap1c, ap2c, \
bpq2, rbpq2, bsq2, \
bpw2, rbpw2, bsw2!");
    }
    x->end_f = x->cur_f;
    x->end_a = x->cur_a;
    x->end_b = x->cur_b;
  }
  return (x);
}

void filter_tilde_setup(void)
{
  filter_tilde_class = class_new(gensym("filter~"), (t_newmethod)filter_tilde_new,
        0, sizeof(t_filter_tilde), 0, A_GIMME, 0);
  CLASS_MAINSIGNALIN(filter_tilde_class, t_filter_tilde, x_msi);
  class_addmethod(filter_tilde_class, (t_method)filter_tilde_dsp, gensym("dsp"), 0);
  class_addmethod(filter_tilde_class, (t_method)filter_tilde_ft1, gensym("ft1"), A_FLOAT, 0);
  class_addmethod(filter_tilde_class, (t_method)filter_tilde_ft2, gensym("ft2"), A_FLOAT, 0);
  class_addmethod(filter_tilde_class, (t_method)filter_tilde_ft3, gensym("ft3"), A_FLOAT, 0);
  class_addmethod(filter_tilde_class, (t_method)filter_tilde_ft4, gensym("ft4"), A_FLOAT, 0);
  class_addmethod(filter_tilde_class, (t_method)filter_tilde_print, gensym("print"), 0);
//  class_sethelpsymbol(filter_tilde_class, gensym("iemhelp/help-filter~"));
}
