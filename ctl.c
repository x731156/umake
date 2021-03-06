# include "ctl.h"
# include "thread.h"
# include "cas.h"
# include "sem.h"
# include "event.h"
# include "analysis.h"
# include "locate.h" 
# include "global.h"
 
// <<ctl_base >> =================================================================================
void ctl_base_init (struct ctl_base **ctlb, struct ctl_ *ctl, struct ctl_pass *ctp)  {
  struct ctl_base *ct = umalloc (sizeof (struct ctl_base));
  ct->cps = ctp;
  ct->ctl = ctl;
  ct->status = 0;
  ct->handle = null;
  ct->pas_wait = null;
  ct->sem_nofity = null;
  ct->sh = null;
  * ctlb = ct;
}
void ctl_base_uninit (struct ctl_base **ctlb) {
  if (ctlb != null)  {
    struct ctl_base *ct = *ctlb;
    *ctlb = null;
    if (ct != null)  {
      shell_desc_uninit (& ct->sh);
      process_colsehandle (& ct->handle);
      ufree (ct);
    }
  }
}
// <<ctl_pass >> =================================================================================
void ctl_pass_init (struct ctl_pass **ctlp, struct ctl_ *ctl, kbool isbase)  {
  struct ctl_pass *ct = umalloc (sizeof (struct ctl_pass));
  ct->child = null;
  ct->as_wait.int_blk = 0;
  ct->cmd_stack.int_blk = 0;
  ct->_shell_cc = 0;
  ct->ctl = ctl;
  ct->evt_attachParnet = null; 
  ct->level_n = ct;
  ct->level_p = ct;
  ct->parent = null;
  ct->_childpass_total = 0;
  ct->_childpass_total_dtor = 0;
  ct->_ptr_parentpass_total = null;
  ct->thread_h = null;
  ct->sem_nofity = null;
  ct->shp_basecount = 0;
  ct->shp_baseptr = null;
  ct->shp_ccmdg = 0;
  ct->cs_cmdgarbage = null;

  event_init (& ct->evt_gate);
  if (isbase == kfalse)  {
    semaphore_init (& ct->sem_nofity, 0);
  }
  * ctlp = ct;
}

void ctl_pass_uninit (struct ctl_pass **ctlp) {
  if (ctlp != null)  {
    struct ctl_pass *ct = *ctlp;
    *ctlp = null;
    if (ct != null)  {
      int id;
      struct ctl_atomic_stack_pn *ap;
      ct->cmd_stack.int_blk &= ~CTL_LEVEL_1_SIGNAL;
      ap = ct->cmd_stack.level_n;
      /*clear atomic cmd-stack */
      for (; ap != null; ) {
        struct ctl_atomic_stack_pn *cc = ap->level_n;
        ctl_base_uninit (& ap->cdb);
        ufree (ap);
        ap = cc;
      }
      ct->as_wait.int_blk &= ~CTL_LEVEL_1_SIGNAL;
      ap = ct->as_wait.level_n;
      /*clear atomic wait-stack */
      for (; ap != null; ) {
        struct ctl_atomic_stack_pn *cc = ap->level_n;
        ctl_base_uninit (& ap->cdb);
        ufree (ap);
        ap = cc;
      }
      for (id = 0; id != ct->shp_ccmdg; id++) {
        ctl_base_uninit (& ct->cs_cmdgarbage[id]->cdb);
        ufree (ct->cs_cmdgarbage[id]);
      }
      event_uninit (& ct->evt_gate);
      thread_close (& ct->thread_h);
      semaphore_uninit (& ct->sem_nofity);
      // list_uninit_ISQ (& ct->chunk_list, ctl_base_exec_infos_uninit);
      ufree (ct);
    }
  }
}

kbool ctl_base_ignore_errs (struct ctl_base *cdb) {
  if ((cdb->sh->flags & AUXILIARY_GENERATE_FLAGS_N))
    return ktrue;
  else 
    return kfalse;
}

kbool ctl_base_new_console (struct ctl_base *cdb) {
  if (cdb->sh->flags & AUXILIARY_GENERATE_FLAGS_C)
    return ktrue;
  else 
    return kfalse;
}

kbool ctl_base_getfirst_deal (struct ctl_ *ctaq) {
  if (store32 ((void *) & ctaq->ib_except, 1) == 0)
    return ktrue;
  else 
    return kfalse;
}


void ctl_setbreak2 (struct ctl_pass *ct)  {
  struct ctl_pass *pn;
  int id;

  if (ct == null)
    return ;
  else if (ktrue)
  {
    /*wake all node, set breakflags */
    or32 (& ct->cmd_stack.int_blk, CTL_LEVEL_1_SIGNAL);
    or32 (& ct->as_wait.int_blk, CTL_LEVEL_1_SIGNAL);
    event_signal (ct->evt_gate);
    semaphore_vrijgevenall (ct->sem_nofity);
  }
  pn = ct->child;
  for (id = 0; id != ct->_childpass_total_dtor; pn = pn->level_n, id++)
  {  ctl_setbreak2 (pn); }
}

void out_errs_cc (struct uchar *cmd, us *errs, int len) {
  us cc;
  us dummy[2] = { _GC ('$'), 0 };
  us *errs2 = dummy;
  static int32_t mspin_lock = 0;

  if (errs != null && (len > 0)) {
    cc = errs[len];
    errs[len] = 0;
    errs2 = errs;
  }
  while (store32(& mspin_lock, 1) != 0)
    _mm_pause ();

  wprintf (_GR ("!!!!!!!!!!! process_create fail !!!!!!!!!!!\n"));
  wprintf (_GR (">>cmd:%ls\n"), cmd?cmd->str:_GR("none"));
  wprintf (_GR (">>system native err:%ls\n"), errs2?errs2:_GR("none"));

  store32(& mspin_lock, 0);

  if (errs != null && (len > 0))
    errs[len] = cc;
  else ;
}

// <sweep-callback>
void ctl_sweep (struct ctl_sweep *sweep, void *stkshared_mem, int membytes) {
  struct ctl_atomic_stack_hd au, av;
  struct ctl_base *cdb;
  struct ctl_atomic_stack_hd *hd;
  struct ctl_atomic_stack_pn *ap;
  struct ctl_ *ctaq;
  int sigc;

  while  (ktrue) {

    /* sleep until exist work */
    hd = & sweep->schedu_stack;
    ctaq = sweep->ctl;
    semaphore_passeren (sweep->sem_gate);

    do {
  QL: sigc = kfalse;
      au.level_n = hd->level_n;
      au.aba_count = hd->aba_count;
      av.level_n = au.level_n;
      av.aba_count = au.aba_count + 1;
      if ( au.int_blk & CTL_LEVEL_2_SIGNAL)
        return ;
      else if ( au.int_blk & CTL_LEVEL_1_SIGNAL)
        sigc = ktrue;
      else if (au.level_n == null)
        achieve QL;
      else 
        av.level_n = au.level_n->level_n;
    } while ( casdp3 ( (void **)  hd, 
                       (void **) & av,
                       (void **) & au ) != ktrue );

    if (sigc != kfalse) {
      /* some kind of exception causes, prior to this, the node 
         that caused the exception  has temporarily handled \
         the situation [blocking  queues ro stacks and its own nodes (depends on the scene where the exception occurred.)]  
         at this point, the node thread may wait for the asynchronous thread to process the result 
         we're going to forward these unhandled requirements to our respective node stacks. */
      do {
        do {
          au.level_n = hd->level_n;
          au.aba_count = hd->aba_count;
          av.level_n = au.level_n;
          av.aba_count = au.aba_count + 1;
          if ( au.int_blk & CTL_LEVEL_1_SIGNAL) {
            if (au.int_blk != CTL_LEVEL_1_SIGNAL) {
              av.level_n = ((struct ctl_atomic_stack_pn *)(au.int_blk & ~((intptr_t) CTL_LEVEL_1_SIGNAL)))->level_n;
              av.int_blk |= (au.int_blk & (intptr_t) CTL_LEVEL_1_SIGNAL);
            }
          } else if (au.level_n != null) {
            av.level_n = au.level_n->level_n;
          }
        } while ( casdp3 ( (void **)  hd, 
                           (void **) & av,
                           (void **) & au ) != ktrue );
        au.int_blk &= ~(intptr_t)CTL_LEVEL_1_SIGNAL;
        if (au.level_n != null) {
          hd = au.level_n->cdb->pas_wait;  /* node 's wait stack  */
          au.level_n->cdb->status = -1;
          ap = au.level_n;
#define cas_insert_private_()                                        \
          do {                                                      \
            intptr_t q;                                             \
            av.level_n = hd->level_n;                               \
            av.aba_count = hd->aba_count;                           \
            q = av.int_blk & CTL_LEVEL_1_SIGNAL;                    \
            av.int_blk &= ~CTL_LEVEL_1_SIGNAL;                      \
            ap->level_n = av.level_n;                               \
            av.int_blk |= q;                                        \
            au.level_n = ap;                                        \
            au.int_blk |= q;                                        \
            au.aba_count = av.aba_count + 1;                        \
          } while (  casdp3 (  (void **)   hd,                      \
                               (void **) & au,                      \
                               (void **) & av ) != ktrue   );       \
          /* pv-post */ \
          semaphore_vrijgeven (* cdb->sem_nofity)               \
          
          cas_insert_private_();
          hd = & sweep->schedu_stack;
        }
        /* post to task node */
      } while (au.level_n != null);

      return ;
    } else {
      cdb = au.level_n ? au.level_n->cdb : (struct ctl_base *) null; 
      assert (cdb != null);
      cdb->status = process_create (  & cdb->handle,   cdb->sh->command, 
         ctl_base_new_console (cdb)  ? ktrue 
          : kfalse,  ktrue  );
      if (cdb->status != 0) {
        struct uchar *q = null;
        sigc = getsys_err_threadlocal (& q);
        assert (sigc == 0);
        out_errs_cc (cdb->sh->command, q->str, q->length);
        uchar_uninit (& q);
        if (ctl_base_ignore_errs (cdb) == kfalse) {
          if (ctl_base_getfirst_deal (ctaq) != kfalse) {
            or32 (& sweep->schedu_stack.int_blk, CTL_LEVEL_1_SIGNAL);
            ctl_setbreak2 (ctaq->root);     
          } 
        }
      } 
      ap = au.level_n;
      hd = cdb->pas_wait;
      cas_insert_private_();
    }
  }
}

finline
void ctl_pass_cmpsignal_parent (struct ctl_pass *ctlpass)  {
  if (add32 (ctlpass->_ptr_parentpass_total, -1) == 1) 
    event_signal (* ctlpass->evt_attachParnet);
  else ;
}

finline
void ctl_pass_push_chunk (struct ctl_pass *ctlpass, struct ctl_atomic_stack_pn *pn)  {
  ctlpass->cs_cmdgarbage[ctlpass->shp_ccmdg++] = pn;
}

// <pass-callback>
void ctl_pass (struct ctl_pass *cpa, void *stkshared_mem, int membytes) {
  
  struct ctl_atomic_stack_hd au, av, *ad, ag;
  struct ctl_atomic_stack_pn *ap, *aq;
  struct ctl_base *cdb;
  struct ctl_sweep *sweep;
  struct ctl_ *ctaq;
  intptr_t sigk;
  int sig, sigc;
  int scan;
  int merge = 0;
  int q;
  int b, d, p;

  scan = 0;
  ctaq = cpa->ctl;
  sweep = ctaq->sweep;
  /* sleep until all sub tree call compelte */
  event_wait (cpa->evt_gate);
  if ( (cpa->cmd_stack.int_blk & CTL_LEVEL_1_SIGNAL)
    || (cpa->as_wait.int_blk & CTL_LEVEL_1_SIGNAL) )
    return  ;
  else;
  /*try popnode */
  do {
    /* set shared stack  */
    cpa->shp_baseptr = stkshared_mem;
    cpa->shp_basecount_re = PASS_CAPACITY_STATUS_BUFFER;
    cpa->cs_cmdgarbage = (void *) & ((uint8_t *)stkshared_mem)[PASS_CAPACITY_STATUS_BUFFER];

    do {
      sigc = kfalse;
      au.level_n = cpa->cmd_stack.level_n;
      au.aba_count = cpa->cmd_stack.aba_count;
      av.level_n = au.level_n;
      av.aba_count = au.aba_count + 1;
      if ( au.int_blk & CTL_LEVEL_1_SIGNAL)
        return ; /*break flags, no need pop node*/
      else if (au.int_blk != 0) {
        if (au.level_n->cdb->sh->flags & AUXILIARY_GENERATE_FLAGS_A) {
          merge ++;
          aq = au.level_n;
          /* merge async request  */
          while (aq->level_n != null 
             && (aq->level_n->cdb->sh->flags & AUXILIARY_GENERATE_FLAGS_A))
             aq = aq->level_n, merge++;
          /* cas write head */
          do {
            ag.level_n = cpa->cmd_stack.level_n;
            ag.aba_count = cpa->cmd_stack.aba_count;
            sigk = ag.int_blk & CTL_LEVEL_1_SIGNAL;
            av.level_n = aq->level_n;
            av.int_blk |= sigk;
            av.aba_count = ag.aba_count + 1;
          } while ( casdp3 ( (void **) & cpa->cmd_stack, 
                             (void **) & av,  
                             (void **) & ag ) != ktrue   ); 
          break ;
        }
        av.level_n = au.level_n->level_n;
      }
    } while ( casdp3 ( (void **) & cpa->cmd_stack, 
                       (void **) & av,  
                       (void **) & au ) != ktrue   );
    
    if (au.level_n == null)  {
        /* wait current async task  */
      while (scan < cpa->_shell_cc) {
        semaphore_passeren (cpa->sem_nofity);

        do {
      QL: au.level_n = cpa->as_wait.level_n;
          au.aba_count = cpa->as_wait.aba_count;
          av.level_n = au.level_n;
          av.aba_count = au.aba_count + 1;
          if ( au.int_blk & CTL_LEVEL_1_SIGNAL)
            return ;
          else if (au.level_n == null)
            achieve QL;
          else 
            av.level_n = au.level_n->level_n;
        } while ( casdp3 ( (void **) & cpa->as_wait, 
                           (void **) & av,
                           (void **) & au ) != ktrue );

        scan ++;
        cdb = au.level_n->cdb;
        cpa->cs_cmdgarbage[cpa->shp_ccmdg++] = au.level_n;

        if (cdb->status != 0) {
          // ctl_pass_appendinfos (cpa, cdb);
          achieve Q3;
        }
        
        sig = process_join (cdb->handle);
        assert (sig == 0);
        sig = process_exitcode (cdb->handle, & cdb->status);
        assert (sig == 0);
        process_rd_pipe_utf8 (  cdb->handle, 
          & cpa->shp_baseptr[cpa->shp_basecount],
          cpa->shp_basecount_re, & q );
        assert (sig == 0);
        cpa->shp_basecount += q;
        cpa->shp_basecount_re -= q;

        if (cdb->status != 0) {
       Q3:if (ctl_base_ignore_errs (cdb)) {
            continue ;
          } else if (ctl_base_getfirst_deal (ctaq) != kfalse) { 
            or32 (& sweep->schedu_stack.int_blk, CTL_LEVEL_1_SIGNAL);
            ctl_setbreak2 (ctaq->root);
          } 
          return  ;
        } 
      }
      /* async task compelte or interrupt */
      ctl_pass_cmpsignal_parent (cpa);
      return  ;
    } else {  
        /*remain sync task or async request to post */
      cdb = au.level_n->cdb;
      assert (cdb != null);
      
      /*outstring|delete files */
      if (cdb->sh->flags & AUXILIARY_GENERATE_FLAGS_S)
        { kprint2 (cdb->sh->command->str); cpa->cs_cmdgarbage[cpa->shp_ccmdg++] = au.level_n; }
      else if (cdb->sh->flags & AUXILIARY_GENERATE_FLAGS_K)
        { file_del (cdb->sh->command->str); cpa->cs_cmdgarbage[cpa->shp_ccmdg++] = au.level_n; }
      else if (cdb->sh->flags & AUXILIARY_GENERATE_FLAGS_P)
# if defined (_WIN32)
        { _wsystem (cdb->sh->command->str); cpa->cs_cmdgarbage[cpa->shp_ccmdg++] = au.level_n; }
# endif 
      else  {
        if (cdb->sh->flags & AUXILIARY_GENERATE_FLAGS_A)  {
            /* throw in an asynchronous \
             scan thread,   push atomic-stack *.*/
          ap = au.level_n;
          ad = & cpa->ctl->sweep->schedu_stack;
          ap->cdb = cdb;

          do {
            au.level_n = ad->level_n;
            au.aba_count = ad->aba_count;
            av.aba_count = au.aba_count + 1;
            if (au.int_blk & CTL_LEVEL_1_SIGNAL) {
              av.level_n = au.level_n;    // nodone.
            } else {
              av.level_n = ap;
              aq->level_n = au.level_n;
            }
          } while (  casdp3 ( (void **)  ad, 
                              (void **)& av,  
                              (void **)& au) != ktrue   );

          if (au.int_blk & CTL_LEVEL_1_SIGNAL) {
             /*sweep will be destroyed   */
            do {                                                      
              intptr_t q;                                             
              av.level_n = cpa->cmd_stack.level_n;                               
              av.aba_count = cpa->cmd_stack.aba_count;                           
              q = av.int_blk & CTL_LEVEL_1_SIGNAL;                    
              av.int_blk &= ~CTL_LEVEL_1_SIGNAL;                      
              aq->level_n = av.level_n;                               
              av.int_blk |= q;                                        
              au.level_n = ap;                                        
              au.int_blk |= q;                                        
              au.aba_count = av.aba_count + 1;                        
            } while (  casdp3 (  (void **) & cpa->cmd_stack,                     
                                 (void **) & au,                      
                                 (void **) & av ) != ktrue   );       
            return  ;
          } 
          /* post sweep gate open n time */
          sig = semaphore_vrijgeven_n (sweep->sem_gate, merge);
          assert (sig == 0);
          continue ;
        } else {
            /* executing in its own thread environment *.*/
          cpa->cs_cmdgarbage[cpa->shp_ccmdg++] = au.level_n;
          cdb->status = process_create (  & cdb->handle,   cdb->sh->command, 
            (cdb->sh->flags & AUXILIARY_GENERATE_FLAGS_C) 
            ? ktrue 
            : kfalse, ktrue  );
          if (cdb->status != 0)  {
             /*errors */
            struct uchar *q = null;
            sig = getsys_err_threadlocal (& q);
            assert (sig == 0);
            out_errs_cc (cdb->sh->command, q->str, q->length);
            uchar_uninit (& q);
            achieve Q7;
          }
          sig = process_join (cdb->handle);
          assert (sig == 0);
          sig = process_exitcode (cdb->handle, & cdb->status);
          assert (sig == 0);
          process_rd_pipe_utf8 (  cdb->handle, 
                       & cpa->shp_baseptr[cpa->shp_basecount],
                       cpa->shp_basecount_re, & q );
          assert (sig == 0);
          cpa->shp_basecount += q;
          cpa->shp_basecount_re -= q;

          if (cdb->status != 0) {
       Q7:  if (ctl_base_ignore_errs (cdb)) {
              achieve L3;
            } else {
              if (ctl_base_getfirst_deal (ctaq) != kfalse) {
                or32 (& sweep->schedu_stack.int_blk, CTL_LEVEL_1_SIGNAL);
                ctl_setbreak2 (ctaq->root);
              } 
              return  ;
            } 
          }
          achieve L3;
        }
      }
  L3: if (++ scan >= cpa->_shell_cc) {
        ctl_pass_cmpsignal_parent (cpa);
        return  ;
      }
    }
  } while (ktrue);
}

void ctl_pass_wait (struct ctl_pass *ct)  {
  struct ctl_pass *pn;
  int id;

  if (ct == null)
    return ;

  pn = ct->child;
  for (id = 0; id != ct->_childpass_total_dtor; pn = pn->level_n, id++)
  {  ctl_pass_wait (pn); }

  if (ct->parent != null) {
    id = thread_join (ct->thread_h);
    assert (id == 0);
  }
}

void ctl_sweep_wait (struct ctl_ *ctaq)  {
  int id;
  int sig;
  for (id = 0; id != ctaq->as_total; id++) {
    sig = thread_join (ctaq->as_handle[id]);
    assert (sig == 0);
  }
}

// <final-callback>
void final_gate (struct ctl_pass *ct, void *stkshared_mem, int membytes) {
  struct ctl_atomic_stack_pn *ap;
  struct ctl_ *ctq = ct->ctl;
  /* sleep until all task compelte */
  event_wait (ct->evt_gate);

  ctl_pass_wait (ctq->root);
  /* when the node thread is complete, the asynchronous scan thread is finished. */
  if ( ctq->sweep->schedu_stack.int_blk & CTL_LEVEL_1_SIGNAL) 
    ctq->exit_code = -1;
  else 
    ctq->sweep->schedu_stack.int_blk |= CTL_LEVEL_2_SIGNAL;

  semaphore_vrijgevenall (ctq->sweep->sem_gate);
  ctl_sweep_wait (ctq);

  /* singal host thread, current task compelte */
  event_signal (* ct->evt_attachParnet);
}

void ctl_startup (struct ctl_pass *ct)  {
  struct ctl_pass *pn;
  int id;

  if (ct == null)
    return ;
  else if (ct->parent == null)
    thread_work (ct->ctl->th_factory, & ct->thread_h, final_gate, ct);
  else 
    thread_work (ct->ctl->th_factory, & ct->thread_h, ctl_pass, ct);
  pn = ct->child;
  for (id = 0; id != ct->_childpass_total_dtor; pn = pn->level_n, id++)
    {  ctl_startup (pn); }
}

void ctl_ignite (struct ctl_pass *ct)  {
  struct ctl_pass *pn;
  int id;

  if (ct == null)
    return ;
  else if (ct->child == null)
  {
    /*wake tail node*/
    event_signal (ct->evt_gate);
  }
  pn = ct->child;
  for (id = 0; id != ct->_childpass_total_dtor; pn = pn->level_n, id++)
  {  ctl_ignite (pn); }
}

void ctl_pass_tree_uninit (struct ctl_pass *pp_)  {

  int id;
  int iq;
  struct ctl_pass *pn = null;
  struct ctl_pass *pg;

  if (pp_ == null)  {
    return  ;
  }

  pg = pp_->child;
  iq = pp_->_childpass_total_dtor;

  ctl_pass_uninit (& pp_);
  /* foreach child node */
  for (id = 0; id != iq; id++, pg = pn)  {
    pn = pg->level_n  ;
    ctl_pass_tree_uninit (pg);
  }
}

void analysis_to_ctl (struct ctl_ **ctl, struct ctl_pass *ct, struct pass_node *pn) {
  struct ctl_atomic_stack_pn *tailc;
  struct ctl_pass *s_dummy;
  struct pass_node *u;
  struct list_v *p;
  int id;
  if (pn == null)
    return  ;
  else if (pn->parent == null)  {
    /* root, init ctl */
    struct ctl_ *c = umalloc (sizeof (struct ctl_));
    int id;
      /*init async scanset  */
    c->sweep = umalloc (sizeof (struct ctl_sweep));
    c->sweep->ctl = c;
    c->sweep->schedu_stack.int_blk = 0;
    semaphore_init (& c->sweep->sem_gate, 0);

    c->as_total = cpu_core_nums ();
    c->as_handle = umalloc (sizeof (struct thread_ *)* c->as_total);
    c->ib_except = 0;
    c->root = null;
    c->exit_code = 0;
    c->mspin_lock = 0;
    c->mp_count = 0;

    * ctl = c;
    thread_init (& c->th_factory, c->as_total);
    ctl_pass_init (& c->root, c, ktrue);
    s_dummy = c->root;

    for (id = 0; id != c->as_total; id++) 
      { thread_work (c->th_factory, & c->as_handle[id], ctl_sweep, c->sweep); } 
  } else  {
    /* init self chunk, link context  */
    ++ (* ctl)->mp_count;
    ctl_pass_init (& s_dummy, * ctl, kfalse);

    if (ct->child == null)  {
      ct->child = s_dummy;
    } else {
      s_dummy->level_n = ct->child;
      s_dummy->level_p = ct->child->level_p;
      ct->child->level_p->level_n = s_dummy;
      ct->child->level_p = s_dummy;
    }
    ct->_childpass_total_dtor ++;
    ct->_childpass_total ++;

    s_dummy->_shell_cc = pn->shell_chain->nums;
    s_dummy->_ptr_parentpass_total = & ct->_childpass_total;
    s_dummy->evt_attachParnet = & ct->evt_gate;
    s_dummy->parent = ct;

    tailc = null;
    /* convert ctl_pass to atomic task  */
    LIST_FOREACH (pn->shell_chain, id, p) {
      struct ctl_base *cb;
      struct ctl_atomic_stack_pn *asp = umalloc (sizeof (struct ctl_atomic_stack_pn));
      ctl_base_init (& cb, *ctl, s_dummy);
      cb->pas_wait = & s_dummy->as_wait;
      cb->sem_nofity = & s_dummy->sem_nofity;
      cb->sh = p->shell_o;
      asp->level_n = null;
      asp->cdb = cb;
      p->shell_o = null;
      if (tailc == null) {
        s_dummy->cmd_stack.level_n = asp;
      } else {
        tailc->level_n = asp;
      }
      tailc = asp;
    }
  }
  
  /* foreach child node  */
  for (id = 0, u = pn->child; id != pn->childs; u = u->level_n, id++)
    { analysis_to_ctl (ctl, s_dummy, u); }
}

void outlog (struct file_ *logh, struct ctl_pass *ct, int *count) {
  struct ctl_pass *pn;
  int id;

  if (ct == null)
    return ;
  else if (ct->parent == null)
  {
    file_write3 ( logh, 
      -1, 
      _QR (">>>>>>result:process %s<<<<<<<\n"), 
      ct->ctl->exit_code 
      ? _QR ("fail") 
      : _QR ("success")  );
  }
  ++ *count;
  pn = ct->child;
  for (id = 0; id != ct->_childpass_total_dtor; pn = pn->level_n, id++)
  {  outlog (logh, pn, count); }

  if (ct->parent != null)
  {
    // out base infos 
    file_write (logh, -1, ct->shp_baseptr, ct->shp_basecount, null);
  }
}

int ctl_do (struct ctl_ **ctl0_) {
  struct event_ *pp_block;
  struct file_ *fh =null;
  struct ctl_ *ctl = * ctl0_;
  struct uchar *bsm = null;
  char uft8_bom[3] = { 0xEF, 0xBB, 0xBF };
  int sig;
  int id;
  us cc[MAX_PATH];
  * ctl0_ = null;
  event_init (& pp_block);
  ctl->root->evt_attachParnet = & pp_block;
  thread_setnums_nosync_bad (ctl->th_factory,  ctl->as_total + 1 + ctl->mp_count);
  ctl_startup (ctl->root);
  ctl_ignite (ctl->root);
  sig = event_wait (pp_block);
  assert (sig == 0);
  thread_join (ctl->root->thread_h);
  getself_dir (& bsm);
  uchar_insert3 (bsm, -1, SLASH_STRING_); 
  uchar_insert3 (bsm, -1, _GR ("umake_reslt.txt"));
  sig = file_open (& fh, bsm->str, FILE2_EXIST_ORCREATE_CLEAR, FILE2_READ_WRITE);
  assert (sig == 0);
  file_write (fh, -1, & uft8_bom[0], sizeof (uft8_bom), null);
  event_uninit (& pp_block);
  id = 0;
  outlog (fh, ctl->root, & id);
  file_close (& fh);
  sig = ctl->exit_code;;
  ctl_pass_tree_uninit (ctl->root);
  /* uninit sweep  */
  ctl->sweep->schedu_stack.int_blk &= ~CTL_LEVEL_1_SIGNAL;
  ctl->sweep->schedu_stack.int_blk &= ~CTL_LEVEL_2_SIGNAL;
  assert (ctl->sweep->schedu_stack.int_blk == 0);
  semaphore_uninit (& ctl->sweep->sem_gate);
  ufree (ctl->sweep);
  for (id = 0; id != ctl->as_total; id++)
    thread_close (& ctl->as_handle[id]);
  thread_uninit (& ctl->th_factory);
  ufree (ctl->as_handle);
  ufree (ctl);
  uchar_uninit (& bsm);
  return sig;
}
// need to test with analysis