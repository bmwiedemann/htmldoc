/*
 * Markdown parsing definitions for HTMLDOC, a HTML document processing program.
 *
 * Copyright 2017 by Michael R Sweet.
 *
 * This program is free software.  Distribution and use rights are outlined in
 * the file "COPYING".
 */

/*
 * Include necessary headers...
 */

#  include "markdown.h"
#  include "mmd.h"


/*
 * Local functions...
 */

static void       add_block(tree_t *hparent, mmd_t *parent);
static void       add_leaf(tree_t *hparent, mmd_t *node);
static uchar      *make_anchor(mmd_t *block);
static uchar      *make_anchor(const uchar *text);


/*
 * 'mdReadFile()' - Read a Markdown file.
 */

tree_t *				/* O - HTML document tree */
mdReadFile(tree_t     *parent,		/* I - Parent node */
           FILE       *fp,		/* I - File to read from */
           const char *base)		/* I - Base path/URL */
{
  mmd_t       *doc = mmdLoadFile(fp);   /* Markdown document */
  tree_t      *html,                    /* HTML element */
              *head,                    /* HEAD element */
              *temp,                    /* META/TITLE element */
              *body;                    /* BODY element */
  const char  *meta;                    /* Title, author, etc. */


  html = htmlAddTree(parent, MARKUP_HTML, NULL);

  head = htmlAddTree(html, MARKUP_HEAD, NULL);
  if ((meta = mmdGetMetadata(doc, "title")) != NULL)
  {
    temp = htmlAddTree(head, MARKUP_TITLE, NULL);
    htmlAddTree(temp, MARKUP_NONE, (uchar *)meta);
  }
  if ((meta = mmdGetMetadata(doc, "author")) != NULL)
  {
    temp = htmlAddTree(head, MARKUP_META, NULL);
    htmlSetVariable(temp, (uchar *)"name", (uchar *)"author");
    htmlSetVariable(temp, (uchar *)"content", (uchar *)meta);
  }
  if ((meta = mmdGetMetadata(doc, "copyright")) != NULL)
  {
    temp = htmlAddTree(head, MARKUP_META, NULL);
    htmlSetVariable(temp, (uchar *)"name", (uchar *)"copyright");
    htmlSetVariable(temp, (uchar *)"content", (uchar *)meta);
  }
  if ((meta = mmdGetMetadata(doc, "version")) != NULL)
  {
    temp = htmlAddTree(head, MARKUP_META, NULL);
    htmlSetVariable(temp, (uchar *)"name", (uchar *)"version");
    htmlSetVariable(temp, (uchar *)"content", (uchar *)meta);
  }

  body = htmlAddTree(html, MARKUP_BODY, NULL);
  add_block(body, doc);

  mmdFree(doc);

  return (html);
}


/*
 * 'add_block()' - Add a block node.
 */

static void
add_block(tree_t *html,                 /* I - Parent HTML node */
          mmd_t  *parent)               /* I - Parent node */
{
  markup_t      element;                /* Enclosing element, if any */
  mmd_t         *node;                  /* Current child node */
  mmd_type_t    type;                   /* Node type */
  tree_t        *block;                 /* Block node */


  switch (type = mmdGetType(parent))
  {
    case MMD_TYPE_BLOCK_QUOTE :
        element = MARKUP_BLOCKQUOTE;
        break;

    case MMD_TYPE_ORDERED_LIST :
        element = MARKUP_OL;
        break;

    case MMD_TYPE_UNORDERED_LIST :
        element = MARKUP_UL;
        break;

    case MMD_TYPE_LIST_ITEM :
        element = MARKUP_LI;
        break;

    case MMD_TYPE_HEADING_1 :
        element = MARKUP_H1;
        break;

    case MMD_TYPE_HEADING_2 :
        element = MARKUP_H2;
        break;

    case MMD_TYPE_HEADING_3 :
        element = MARKUP_H3;
        break;

    case MMD_TYPE_HEADING_4 :
        element = MARKUP_H4;
        break;

    case MMD_TYPE_HEADING_5 :
        element = MARKUP_H5;
        break;

    case MMD_TYPE_HEADING_6 :
        element = MARKUP_H6;
        break;

    case MMD_TYPE_PARAGRAPH :
        element = MARKUP_P;
        break;

    case MMD_TYPE_CODE_BLOCK :
        block = htmlAddTree(html, MARKUP_PRE, NULL);

        for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
          htmlAddTree(block, MARKUP_NONE, (uchar *)mmdGetText(node));
        return;

    case MMD_TYPE_THEMATIC_BREAK :
        htmlAddTree(html, MARKUP_HR, NULL);
        return;

    default :
        element = MARKUP_NONE;
        break;
  }

  if (element != MARKUP_NONE)
    block = htmlAddTree(html, element, NULL);
  else
    block = html;

  if (type >= MMD_TYPE_HEADING_1 && type <= MMD_TYPE_HEADING_6)
  {
   /*
    * Add an anchor for each heading...
    */

    block = htmlAddTree(block, MARKUP_A, NULL);
    htmlSetVariable(block, (uchar *)"id", make_anchor(parent));
  }

  for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
  {
    if (mmdIsBlock(node))
      add_block(block, node);
    else
      add_leaf(block, node);
  }
}


/*
 * 'add_leaf()' - Add a leaf node.
 */

static void
add_leaf(tree_t *html,                  /* I - Parent HTML node */
         mmd_t  *node)                  /* I - Leaf node */
{
  tree_t        *parent;                /* HTML node for this text */
  markup_t      element;                /* HTML element for this text */
  uchar         buffer[1024],           /* Text with any added whitespace */
                *text,                  /* Text to write */
                *url;                   /* URL to write */


  text = (uchar *)mmdGetText(node);
  url  = (uchar *)mmdGetURL(node);

  switch (mmdGetType(node))
  {
    case MMD_TYPE_EMPHASIZED_TEXT :
        element = MARKUP_EM;
        break;

    case MMD_TYPE_STRONG_TEXT :
        element = MARKUP_STRONG;
        break;

    case MMD_TYPE_STRUCK_TEXT :
        element = MARKUP_DEL;
        break;

    case MMD_TYPE_LINKED_TEXT :
        element = MARKUP_A;
        break;

    case MMD_TYPE_CODE_TEXT :
        element = MARKUP_CODE;
        break;

    case MMD_TYPE_IMAGE :
        if (mmdGetWhitespace(node))
          htmlAddTree(html, MARKUP_NONE, (uchar *)" ");

        parent = htmlAddTree(html, MARKUP_IMG, NULL);
        htmlSetVariable(parent, (uchar *)"src", url);
        if (text)
          htmlSetVariable(parent, (uchar *)"alt", text);
        return;

    case MMD_TYPE_HARD_BREAK :
        htmlAddTree(html, MARKUP_BR, NULL);
        return;

    case MMD_TYPE_SOFT_BREAK :
        htmlAddTree(html, MARKUP_WBR, NULL);
        return;

    case MMD_TYPE_METADATA_TEXT :
        return;

    default :
        element = MARKUP_NONE;
        break;
  }

  if (element == MARKUP_NONE)
    parent = html;
  else if ((parent = html->last_child) == NULL || parent->markup != element)
  {
    parent = htmlAddTree(html, element, NULL);

    if (element == MARKUP_A && url)
    {
      if (!strcmp((char *)url, "@"))
        htmlSetVariable(parent, (uchar *)"href", make_anchor(text));
      else
        htmlSetVariable(parent, (uchar *)"href", url);
    }
  }

  if (mmdGetWhitespace(node))
  {
    buffer[0] = ' ';
    strlcpy((char *)buffer + 1, (char *)text, sizeof(buffer) - 1);
    text = buffer;
  }

  htmlAddTree(parent, MARKUP_NONE, text);
}


/*
 * 'make_anchor()' - Make an anchor for internal links from a block node.
 */

static uchar *                          /* O - Anchor string */
make_anchor(mmd_t *block)               /* I - Block node */
{
  mmd_t         *node;                  /* Current child node */
  const char    *text;                  /* Text from block */
  uchar         *bufptr;                /* Pointer into buffer */
  static uchar  buffer[1024];           /* Buffer for anchor string */


  for (bufptr = buffer, node = mmdGetFirstChild(block); node; node = mmdGetNextSibling(node))
  {
    if (mmdGetWhitespace(node) && bufptr < (buffer + sizeof(buffer) - 1))
      *bufptr++ = '-';
    for (text = mmdGetText(node); text && *text && bufptr < (buffer + sizeof(buffer) -1); text ++)
    {
      if ((*text >= '0' && *text <= '9') || (*text >= 'a' && *text <= 'z') || (*text >= 'A' && *text <= 'Z') || *text == '.' || *text == '-')
        *bufptr++ = (uchar)tolower(*text);
      else if (*text == ' ')
        *bufptr++ = '-';
    }
  }

  *bufptr = '\0';

  return (buffer);
}


/*
 * 'make_anchor()' - Make an anchor for internal links from text.
 */

static uchar *                          /* O - Anchor string */
make_anchor(const uchar *text)          /* I - Text */
{
  uchar         *bufptr;                /* Pointer into buffer */
  static uchar  buffer[1024];           /* Buffer for anchor string */


  for (bufptr = buffer; *text && bufptr < (buffer + sizeof(buffer) - 1); text ++)
  {
    if ((*text >= '0' && *text <= '9') || (*text >= 'a' && *text <= 'z') || (*text >= 'A' && *text <= 'Z') || *text == '.' || *text == '-')
      *bufptr++ = (uchar)tolower(*text);
    else if (*text == ' ')
      *bufptr++ = '-';
  }

  *bufptr = '\0';

  return (buffer);
}
