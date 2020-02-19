#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_string.h>
#include <ngx_stream.h>


ngx_module_t  ngx_stream_mytest_module;

typedef struct {
    size_t conn_count;
} ngx_stream_mytest_main_conf_t;

typedef struct {
	ngx_str_t mytest;
	ngx_msec_t timeout;
} ngx_stream_mytest_srv_conf_t;

typedef struct {
    ngx_pool_t *pool;
    ngx_stream_mytest_srv_conf_t *rscf;
    ngx_stream_session_t *s;
    ngx_buf_t buf;
} ngx_stream_mytest_session_t;

#define MYTEST_BUFSIZE NGX_DEFAULT_POOL_SIZE/2

static void ngx_stream_mytest_handler(ngx_stream_session_t * s);

static void ngx_stream_mytest_reply(ngx_event_t * wev)
{
	ngx_connection_t *c;
	ngx_stream_session_t *s;
	ssize_t n;
	size_t size;
	ngx_err_t err;
	ngx_stream_mytest_session_t *ctx;
	ngx_stream_mytest_srv_conf_t *rscf;
	c = wev->data;
	s = c->data;
    ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "IN ngx_stream_mytest_reply");
	
	
	rscf = ngx_stream_get_module_srv_conf(s, ngx_stream_mytest_module);				

	ctx=ngx_stream_get_module_ctx(s, ngx_stream_mytest_module);
    size = ctx->buf.last - ctx->buf.pos;
    if (size == 0) {
    	ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "OUT ngx_stream_mytest_reply 3");	    	
        return;
    }
	n = c->send(c, ctx->buf.pos, size);
	err = ngx_socket_errno;
	ngx_log_debug1(NGX_LOG_DEBUG_STREAM, c->log, 0, "send(): %z", n);
	if (n == NGX_ERROR) {
		if (err == NGX_EAGAIN) {
			wev->ready = 0;

			if (!wev->timer_set) {
				ngx_add_timer(wev, ctx->rscf->timeout);
			}

			if (ngx_handle_write_event(wev, 0) != NGX_OK) {
				ngx_stream_finalize_session(s,
					NGX_STREAM_INTERNAL_SERVER_ERROR);
			}
    	ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "OUT ngx_stream_mytest_reply 2");	
			return;
		}

		ngx_connection_error(c, err, "send() failed");

		ngx_stream_finalize_session(s, NGX_STREAM_OK);
    ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "OUT ngx_stream_mytest_reply 1");			
		return;
	}

	if (wev->timer_set) {
		ngx_del_timer(wev);
	}

  if (n > 0) {
      ctx->buf.pos += n;
      size -= n;
      if (size == 0) {
          /* reset to process next rpc call */
          ctx->buf.pos = ctx->buf.start;
          ctx->buf.last = ctx->buf.start;
//          c->read->handler = ngx_stream_mytest_reply;
      }
  }
    ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "OUT ngx_stream_mytest_reply");	
}

static int send_reply(ngx_stream_mytest_session_t *ctx)
{
    ngx_connection_t *c;	
    ngx_stream_session_t *s;
	    s = ctx->s;
    c = s->connection;	
	if(ctx->buf.start==NULL){
        ngx_log_error(NGX_LOG_ERR, c->log, 0, "mytest buffer size is %d ", MYTEST_BUFSIZE);
        ngx_stream_finalize_session(s, NGX_STREAM_BAD_REQUEST);
        return -1;

	}

	if((ctx->buf.last-ctx->buf.start) > MYTEST_BUFSIZE)
	{
        ngx_log_error(NGX_LOG_ERR, c->log, 0, "mytest buffer size is %d ", MYTEST_BUFSIZE);
        ngx_stream_finalize_session(s, NGX_STREAM_BAD_REQUEST);
        return -1;
	}

	c = ctx->s->connection;
    c->write->handler = ngx_stream_mytest_reply;

    c->write->handler(c->write);
	return NGX_OK;
}


static void ngx_stream_echo(ngx_event_t * rev)
{
//		u_char buf[256]={0};
		int rc;
    size_t size;
    ssize_t n;
    ngx_err_t err;
    ngx_connection_t *c;
    ngx_stream_session_t *s;
    ngx_stream_mytest_srv_conf_t *rscf;
	ngx_stream_mytest_session_t *ctx;	
    c = rev->data;
    s = c->data;	
   ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "IN ngx_stream_echo");		    
    
	ctx=ngx_stream_get_module_ctx(s, ngx_stream_mytest_module);
	if(ctx->buf.start==NULL){
		u_char *p;
        p = ngx_pnalloc(c->pool, MYTEST_BUFSIZE);
        if (p == NULL) {
            ngx_stream_finalize_session(s,NGX_STREAM_INTERNAL_SERVER_ERROR);
            ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "OUT ngx_stream_echo");		    
            return;
        }
        ctx->buf.start = p;
        ctx->buf.end = p + MYTEST_BUFSIZE;
        ctx->buf.pos = p;
        ctx->buf.last = p;
	}


    ngx_log_debug0(NGX_LOG_DEBUG_STREAM, c->log, 0,  "stream mytest handler");
    rscf = ngx_stream_get_module_srv_conf(s, ngx_stream_mytest_module);
    if (rev->timedout) {
        ngx_log_error(NGX_LOG_INFO, c->log, NGX_ETIMEDOUT, "client timed out");
        ngx_stream_finalize_session(s, NGX_STREAM_OK);
                 ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "OUT ngx_stream_echo 1");      
        return;
    }
#if 0
    n = recv(c->fd, (char *)buf, 2, MSG_PEEK);
    err = ngx_socket_errno;
    ngx_log_debug1(NGX_LOG_DEBUG_STREAM, c->log, 0, "recv(): %z", n);

    if (n == -1) {
        if (err == NGX_EAGAIN) {
            rev->ready = 0;

            if (!rev->timer_set) {
                rscf = ngx_stream_get_module_srv_conf(s, ngx_stream_mytest_module);
                ngx_add_timer(rev, rscf->timeout);
            }

            if (ngx_handle_read_event(rev, 0) != NGX_OK) {
                ngx_stream_finalize_session(s,  NGX_STREAM_INTERNAL_SERVER_ERROR);
            }
                 ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "OUT ngx_stream_echo 2");      
            return;
        }
        ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "OUT ngx_stream_echo 3");  
        ngx_connection_error(c, err, "recv() failed");

        ngx_stream_finalize_session(s, NGX_STREAM_OK);
        return;
    }
#endif

    if (rev->timer_set) {
        ngx_del_timer(rev);
    }
	
	size=MYTEST_BUFSIZE;
	n = c->recv(c, ctx->buf.last, size);
    err = ngx_socket_errno;
    ngx_log_debug1(NGX_LOG_DEBUG_STREAM, c->log, 0, "recv(): %z", n);
   
    if (n == NGX_ERROR) {
        if (err == NGX_EAGAIN) {
            rev->ready = 0;

            if (!rev->timer_set) {
                ngx_add_timer(rev, ctx->rscf->timeout);
            }

            if (ngx_handle_read_event(rev, 0) != NGX_OK) {
                ngx_stream_finalize_session(s,
                    NGX_STREAM_INTERNAL_SERVER_ERROR);
            }
                 ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "OUT ngx_stream_echo 5"); 
            return;
        }

        ngx_connection_error(c, err, "recv() failed");

        ngx_stream_finalize_session(s, NGX_STREAM_OK);
                 ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "OUT ngx_stream_echo 6");         
        return;
    }
    
    if(n<0){
           rev->ready = 0;

            if (!rev->timer_set) {
                ngx_add_timer(rev, ctx->rscf->timeout);
            }

            if (ngx_handle_read_event(rev, 0) != NGX_OK) {
                ngx_stream_finalize_session(s,
                    NGX_STREAM_INTERNAL_SERVER_ERROR);
            }
                 ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "OUT ngx_stream_echo a"); 
            return;   	
    }

    if (rev->timer_set) {
        ngx_del_timer(rev);
    }

    if (n == 0) {
        rev->eof = 1;
        ngx_log_error(NGX_LOG_INFO, c->log, 0, "client closed");
        ngx_stream_finalize_session(s, NGX_STREAM_OK);
                        ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "OUT ngx_stream_echo 7"); 
        return;
    }
    if (n > 0) {
        ctx->buf.last += n;
        rc = send_reply(ctx);
        if (rc == NGX_ERROR) {
            ngx_log_error(NGX_LOG_ERR, c->log, 0, "reply bad");
            ngx_stream_finalize_session(s, NGX_STREAM_BAD_REQUEST);
                ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "OUT ngx_stream_echo REPLY BAD"); 
        }
    }	
     ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "OUT ngx_stream_echo 8");     
    return;

}


static void
ngx_cleanup_mytest_session(void *data)
{
    ngx_stream_mytest_session_t *ctx = data;
    	  ngx_connection_t *c=ctx->s->connection;
     ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "IN ngx_cleanup_mytest_session ");  
    if (!ctx)
        return;

    ngx_destroy_pool(ctx->pool);
}


static void
ngx_stream_mytest_handler(ngx_stream_session_t * s)
{
	  ngx_connection_t *c;
    ngx_stream_mytest_srv_conf_t *rscf;
    ngx_event_t *rev;

	ngx_stream_mytest_session_t *ctx;
		c = s->connection;
			rscf = ngx_stream_get_module_srv_conf(s, ngx_stream_mytest_module);
            ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "in ngx_stream_mytest_handler");	
		ctx = ngx_pcalloc(c->pool, sizeof(ngx_stream_mytest_session_t));
		if (ctx == NULL) {
			ngx_stream_finalize_session(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
            ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "OUT ngx_stream_mytest_handler");			
			return;
		}
		ctx->s = s;
		ctx->rscf = rscf;
		ctx->pool = ngx_create_pool(NGX_DEFAULT_POOL_SIZE, c->log);
		if (ctx->pool == NULL) {
			ngx_stream_finalize_session(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
			return;
		} else {
			ngx_pool_cleanup_t *cln;
		
			/* register to cleanup our pool */
			cln = ngx_pool_cleanup_add(c->pool, 0);
			cln->handler = ngx_cleanup_mytest_session;
			cln->data = ctx;
		}
		
		ngx_stream_set_ctx(s, ctx, ngx_stream_mytest_module);

	

		c = s->connection;
   	rev = c->read;
		rev->handler=ngx_stream_echo;
		rev->handler(rev);
    ngx_log_error(NGX_LOG_DEBUG, c->log, 0, "OUT ngx_stream_mytest_handler");		
}



static char *
ngx_stream_set_mytest(ngx_conf_t * cf, ngx_command_t * cmd, void *conf)
{
    ngx_stream_core_srv_conf_t *cscf;

    cscf = ngx_stream_conf_get_module_srv_conf(cf, ngx_stream_core_module);

    cscf->handler = ngx_stream_mytest_handler;

    return NGX_CONF_OK;
}

static void *
ngx_stream_mytest_create_main_conf(ngx_conf_t *cf)
{
    ngx_stream_mytest_main_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_stream_mytest_main_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->conn_count = NGX_CONF_UNSET_SIZE;

    return conf;
}


static void *
ngx_stream_mytest_create_srv_conf(ngx_conf_t * cf)
{
    ngx_stream_mytest_srv_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_stream_mytest_srv_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->timeout = NGX_CONF_UNSET_MSEC;

    return conf;
}


static ngx_command_t ngx_stream_mytest_commands[] = {

    {ngx_string("mytest"),
            NGX_STREAM_SRV_CONF | NGX_CONF_NOARGS,
            ngx_stream_set_mytest,
            NGX_STREAM_SRV_CONF_OFFSET,
            0,
        NULL},

    {ngx_string("timeout"),
            NGX_STREAM_SRV_CONF | NGX_CONF_TAKE1,
            ngx_conf_set_msec_slot,
            NGX_STREAM_SRV_CONF_OFFSET,
            offsetof(ngx_stream_mytest_srv_conf_t, timeout),
        NULL},

    ngx_null_command
};

static ngx_stream_module_t ngx_stream_mytest_module_ctx = {
    NULL,                       /* preconfiguration */
    NULL,                       /* postconfiguration */
    ngx_stream_mytest_create_main_conf, /* create main configuration */
    NULL,                       /* init main configuration */
    ngx_stream_mytest_create_srv_conf, /* create server configuration */
    NULL                        /* merge server configuration */
};

ngx_module_t ngx_stream_mytest_module = {
    NGX_MODULE_V1,
    &ngx_stream_mytest_module_ctx, /* module context */
    ngx_stream_mytest_commands,    /* module directives */
    NGX_STREAM_MODULE,          /* module type */
    NULL,                       /* init master */
    NULL,                       /* init module */
    NULL,						/* init process */
    NULL,                       /* init thread */
    NULL,                       /* exit thread */
    NULL,						/* exit process */
    NULL,                       /* exit master */
    NGX_MODULE_V1_PADDING
};

