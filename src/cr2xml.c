/*
*  crmerge - a tool for merging Eressea CR files.
 *  Copyright (C) 2006 Enno Rehling
*
 * This file is part of crtools.
*
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
*
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "config.h"
#include "crparse.h"
#include "hierarchy.h"

/* libxml includes */
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/parser.h>
#include <iconv.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int verbose = 0;

static char *
convert(iconv_t conv, const char * inbuf)
{
  size_t outsize = 2;
  size_t bufsize = 2;
  char *buffer = NULL, *outbuf = NULL;
  size_t insize = strlen(inbuf)+1;
  do {
    outsize += bufsize;
    bufsize = bufsize * 2;
    buffer = (char*)realloc(buffer, bufsize);
    outbuf = buffer+bufsize-outsize;
    iconv(conv, (char**)&inbuf, &insize, &outbuf, &outsize);
  } while (insize>0);
  return buffer;
}

static char *
from_utf8(const char * inbuf)
{
  static iconv_t conv = 0;
  if (conv==0) conv = iconv_open("ISO-8859-1", "UTF-8");
  if (inbuf==NULL) {
    iconv_close(conv);
    conv = 0;
    return NULL;
  }
  return convert(conv, inbuf);
}

static char *
to_utf8(const char * inbuf)
{
  static iconv_t conv = 0;
  if (conv==0) conv = iconv_open("UTF-8", "ISO-8859-1");
  if (inbuf==NULL) {
    iconv_close(conv);
    conv = 0;
    return NULL;
  }
  return convert(conv, inbuf);
}

static blocktype *
xml_readhierarchy(xmlNodePtr root, blocktype * parent)
{
  xmlNodePtr node = root->children;
  xmlChar * name = xmlGetProp(root, BAD_CAST "name");
  blocktype * btype = make_type((const char*)name, parent, NULL, 0);

  assert(root->type==XML_ELEMENT_NODE);
  assert(name!=NULL);

  while (node!=NULL) {
    switch (node->type) {
    case XML_ELEMENT_NODE:
      xml_readhierarchy(node, btype);
      break;
    default:
      break;
    }
    node = node->next;
  }
  xmlFree(name);
  return btype;
}

void
read_cr(parse_info * parser, const char * filename)
{
  FILE * in = fopen(filename, "rt+");
  if (!in) {
    perror(filename);
    return;
  }
  if (verbose) fprintf(stderr, "reading %s\n", filename);

  cr_parse(parser, in);
}

int
usage(const char * name)
{
  fprintf(stderr, "usage: %s [options] [infiles]\n", name);
  fprintf(stderr, "options:\n"
    " -h       display this information\n"
    " -H file  read cr-hierarchy from file\n"
    " -V       print version information\n"
    " -v       verbose\n"
    " -c       create compact XML output\n"
    " -o file  write output to file (default is stdout)\n"
    "infiles:\n"
    " one or more cr-files. if none specified, read from stdin\n");
  return -1;
}

typedef struct xml_context {
  xmlDocPtr doc;
  xmlNodePtr previous;
  blocktype * blocktypes;
} xml_context;

typedef struct block_name {
  char * cr_name;
  char * xml_name;
  struct block_name * next;
} block_name;

static block_name * names = NULL;

/* TODO: this belongs in an external xml configuration file */
static const char * translate[][2] = {
  { "PARTEI", "faction" },
  { "EINHEIT", "unit" },
  { "GEGENSTAENDE", "items" },
  { "DURCHSCHIFFUNG", "traffic" },
  { "BURG", "building" },
  { "SCHIFF", "ship" },
  { "ALLIANZ", "ally" },
  { 0, 0 },
};

static block_name *
get_names(void)
{
  if (names==NULL) {
    int i;
    for (i=0;translate[i][0];++i) {
      block_name * name = malloc(sizeof(block_name));
      name->cr_name = strdup(translate[i][0]);
      name->xml_name = strdup(translate[i][1]);
      name->next = names;
      names = name;
    }
  }
  return names;
}

static xmlChar *
xml_block(const char * cr_name)
{
  block_name * p = get_names();
  while (p && strcmp(p->cr_name, cr_name)!=0) p=p->next;
  if (p) {
    return (xmlChar *)to_utf8(p->xml_name);
  } else {
    char buffer[64], * c;
    strncpy(buffer, cr_name, sizeof(buffer));
    buffer[sizeof(buffer)-1] = 0;
    for (c=buffer;*c;++c) *c = tolower(*c);
    return to_utf8(buffer);
  }
}

static const blocktype *
node_type(context_t context, const char * name)
{
  xml_context * ctx = (xml_context*)context;
  xmlNodePtr parent = ctx->previous;
  const blocktype * type = ctx->blocktypes;
  const blocktype * ptype = parent?(const blocktype *)parent->_private:NULL;

  if (ptype) type = find_type_rel(name, ptype);
  if (type==NULL) {
    if (ptype) fprintf(stderr, "block type %s is unknown, creating as sub-block of %s. Fix hierarchy-file\n", name, ptype->name);
    type = make_type(name, (blocktype*)ptype, NULL, 0);
  }
  return type;
}

static block_t
create_compact_block(context_t context, const char * name, const int * ids, size_t size)
{
  xmlChar * bname = xml_block(name);
  xmlNodePtr node = xmlNewNode(NULL, bname);
  free(bname);

  node->_private = (void*)node_type(context, name);

  if (size>0) {
    char * cids = malloc(13*size);
    char * p = cids;
    int i = 0;
    while (i!=size) {
      sprintf(p, "%d", ids[i]);
      if (++i!=size) {
        p += strlen(p);
        *p++ = ' ';
      }
    }
    xmlSetProp(node, BAD_CAST "id", BAD_CAST cids);
    free(cids);
  }

  return (block_t)node;
}

static block_t
create_block(context_t context, const char * name, const int * ids, size_t size)
{
  xmlNodePtr node = xmlNewNode(NULL, "block");
  xmlChar * cname = (xmlChar *)to_utf8(name);
  node->_private = (void*)node_type(context, name);

  if (size>0) {
    int i;
    for (i=0;i!=size;++i) {
      char zText[12];
      sprintf(zText, "%d", ids[i]);
      xmlNewTextChild(node, NULL, BAD_CAST "id", BAD_CAST zText);
    }
  }
  xmlSetProp(node, BAD_CAST "name", BAD_CAST cname);

  free(cname);
  return (block_t)node;
}

static void
destroy_block(context_t context, block_t bt)
{
  assert(!"didn't think this was used");
}

static void
add_block(context_t context, block_t bt)
{
  xml_context * ctx = (xml_context*)context;
  xmlNodePtr parent = ctx->previous;
  xmlNodePtr node = (xmlNodePtr)bt;
  if (parent!=NULL) {
    const blocktype * type = (const blocktype *)node->_private;
    assert(type);
    while (parent->_private!=type->parent) {
      parent = parent->parent;
    }
    xmlAddChild(parent, node);
  } else {
    xmlDocSetRootElement(ctx->doc, node);
  }
  ctx->previous = node;
}

static block_t
find_block(context_t context, block_t root, const char * name, const int * ids, size_t size)
{
  assert(!"didn't think this was used");
  return NULL;
}

static report_interface xml_ireport = {
  create_block,
  destroy_block,
  add_block,
  find_block
};

static void
compact_set_int(context_t context, block_t bt, const char *name, int i)
{
  xmlNodePtr node = (xmlNodePtr)bt;
  char value[12];
  char * cname = to_utf8(name);

  sprintf(value, "%d", i);
  xmlSetProp(node, BAD_CAST cname, value);
  free(cname);
}

static void
block_set_int(context_t context, block_t bt, const char *name, int i)
{
  xmlNodePtr node = (xmlNodePtr)bt;
  xmlNodePtr param = xmlAddChild(node, xmlNewNode(0, BAD_CAST "int"));
  char value[12];
  xmlChar* cname = (xmlChar*)to_utf8(name);

  sprintf(value, "%d", i);
  xmlSetProp(param, BAD_CAST "name", cname);
  xmlSetProp(param, BAD_CAST "value", BAD_CAST value);
  free(cname);
}

static void
block_set_ints(context_t context, block_t bt, const char *name, const int * ip, size_t size)
{
  xmlNodePtr node = (xmlNodePtr)bt;
  xmlNodePtr param = xmlAddChild(node, xmlNewNode(0, BAD_CAST "list"));
  char value[12];
  xmlChar* cname = (xmlChar*)to_utf8(name);
  int i;

  xmlSetProp(param, BAD_CAST "name", cname);
  for (i=0;i!=size;++i) {
    xmlNodePtr iparam = xmlAddChild(param, xmlNewNode(0, BAD_CAST "int"));
    sprintf(value, "%d", ip[i]);
    xmlSetProp(iparam, BAD_CAST "value", BAD_CAST value);
  }
  free(cname);
}

static void
block_set_string(context_t context, block_t bt, const char * name, const char * value)
{
  xmlNodePtr node = (xmlNodePtr)bt;
  xmlNodePtr param = xmlAddChild(node, xmlNewNode(0, BAD_CAST "string"));

  xmlChar * cname = (xmlChar *)to_utf8(name);
  xmlChar * cvalue = (xmlChar *)to_utf8(value);

  xmlSetProp(param, BAD_CAST "name", BAD_CAST cname);
  xmlSetProp(param, BAD_CAST "value", BAD_CAST cvalue);

  free(cname);
  free(cvalue);
}

static void
complex_set_string(context_t context, block_t bt, const char * name, const char * value)
{
  xmlNodePtr node = (xmlNodePtr)bt;
  char * cname = to_utf8(name);
  char * cvalue = to_utf8(value);

  xmlSetProp(node, BAD_CAST cname, BAD_CAST cvalue);

  free(cname);
  free(cvalue);
}

static void
block_set_entry(context_t context, block_t bt, const char * value)
{
  xmlNodePtr node = (xmlNodePtr)bt;
  xmlNodePtr param = xmlAddChild(node, xmlNewNode(0, BAD_CAST "entry"));
  xmlChar* cvalue = (xmlChar*)to_utf8(value);

  xmlSetProp(param, BAD_CAST "value", cvalue);
  free(cvalue);
}

block_interface xml_iblock = {
  block_set_int,
  block_set_ints,
  block_set_string,
  block_set_entry
};

int
main(int argc, char ** argv)
{
  FILE * f = NULL;
  const char *hierarchy = 0, *infile = 0, *outfile = 0;
  int i;
  parse_info * parser = calloc(1, sizeof(parse_info));

  xml_context context;
  context.blocktypes = NULL;
  context.doc = xmlNewDoc("1.0");
  context.previous = NULL;

  parser->iblock = &xml_iblock;
  parser->ireport = &xml_ireport;

  for (i=1;i!=argc;++i) if (argv[i][0]=='-') {
    switch(argv[i][1]) {
      case 'v':
        verbose = 1;
        break;
      case 'V' :
        fprintf(stderr, "cr2xml\nCopyright (C) 2000 Enno Rehling\n\nThis program comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it\nunder certain conditions; consult the file gpl.txt for details.\n\n");
        fprintf(stderr, "compiled at %s on %s\n", __TIME__, __DATE__);
        break;
      case 'c' :
        xml_ireport.create = create_compact_block;
        xml_iblock.set_int = compact_set_int;
        xml_iblock.set_string = complex_set_string;
        break;
      case 'h' :
        return usage(argv[0]);
      case 'H':
        hierarchy = argv[++i];
        break;
      case 'i' :
        infile = argv[++i];
        break;
      case 'o' :
        outfile = argv[++i];
        break;
      default :
        fprintf(stderr, "Ignoring unknown option.");
        break;
    }
  }

  if (hierarchy!=NULL) {
    xmlDocPtr doc = xmlReadFile(hierarchy, "", 0);
    context.blocktypes = xml_readhierarchy(doc->children, NULL);
  }
  if (infile!=NULL) f = fopen(infile, "rt");
  if (f==NULL) {
    if (verbose) fprintf(stderr, "reading from stdin\n");
    f = stdin;
  }
  parser->bcontext = (context_t)&context;
  {
    cr_parse(parser, f);
  }
  if (f!=stdin) fclose(f);

  xmlSaveFormatFile(outfile, context.doc, 1);

  to_utf8(NULL);
  from_utf8(NULL);
  return 0;
}
