/* gn-macro.h
 *
 * Copyright 2018 Mohammed Sadiq <sadiq@sadiqpk.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>

G_BEGIN_DECLS

static inline xmlDoc *
xml_doc_new (const gchar *data,
             gint         size)
{
  return xmlReadMemory (data, size , "", "utf-8",
                        XML_PARSE_RECOVER | XML_PARSE_NONET);
}

static inline void
xml_doc_free (xmlDoc *doc)
{
  xmlFreeDoc (doc);
}

static inline xmlNode *
xml_doc_get_root_element (xmlDoc *doc)
{
  return xmlDocGetRootElement (doc);
}

static inline xmlTextReader *
xml_reader_new (const gchar *data,
                gint         size)
{
  return xmlReaderForMemory (data, size, "", "utf-8",
                             XML_PARSE_RECOVER | XML_PARSE_NONET);
}

static inline void
xml_reader_free (xmlTextReader *reader)
{
  xmlFreeTextReader (reader);
}

static inline gint
xml_reader_read (xmlTextReader *reader)
{
  return xmlTextReaderRead (reader);
}

static inline xmlDoc *
xml_reader_get_doc (xmlTextReader *reader)
{
  return xmlTextReaderCurrentDoc (reader);
}

static inline const gchar *
xml_reader_get_name (xmlTextReader *reader)
{
  return (const gchar *)xmlTextReaderConstName (reader);
}

static inline const gchar *
xml_reader_get_value (xmlTextReader *reader)
{
  return (const gchar *)xmlTextReaderConstValue (reader);
}

static inline gint
xml_reader_get_node_type (xmlTextReader *reader)
{
  return xmlTextReaderNodeType (reader);
}

static inline gchar *
xml_reader_dup_inner_xml (xmlTextReader *reader)
{
  return (gchar *)xmlTextReaderReadInnerXml (reader);
}

static inline gchar *
xml_reader_dup_string (xmlTextReader *reader)
{
  return (gchar *)xmlTextReaderReadString (reader);
}

static inline xmlBuffer *
xml_buffer_new (void)
{
  return xmlBufferCreate ();
}

static inline xmlTextWriter *
xml_writer_new (xmlBuffer *buffer)
{
  return xmlNewTextWriterMemory (buffer, 0);
}

static inline gint
xml_writer_start_doc (xmlTextWriter *writer)
{
  return xmlTextWriterStartDocument (writer, "1.0", "utf-8", NULL);
}

static inline gint
xml_writer_start_tag (xmlTextWriter *writer,
                      const gchar   *tag)
{
  return xmlTextWriterStartElement (writer, (const xmlChar *)tag);
}

static inline gint
xml_writer_end_tag (xmlTextWriter *writer)
{
  return xmlTextWriterEndElement (writer);
}

static inline gint
xml_writer_write_raw (xmlTextWriter *writer,
                      const gchar   *str)
{
  return xmlTextWriterWriteRaw (writer, (const xmlChar *)str);
}

static inline gint
xml_writer_write_string (xmlTextWriter *writer,
                         const gchar   *str)
{
  return xmlTextWriterWriteString (writer, (const xmlChar *)str);
}

static inline void
xml_writer_add_tag (xmlTextWriter *writer,
                    const gchar   *tag_name,
                    const gchar   *content)
{
  xml_writer_write_raw (writer, "\n");
  xml_writer_start_tag (writer, tag_name);
  xml_writer_write_string (writer, content);
  xml_writer_end_tag (writer);
}

static inline gint
xml_writer_write_attribute (xmlTextWriter *writer,
                            const gchar   *prefix,
                            const gchar   *name,
                            const gchar   *content)
{
  return xmlTextWriterWriteAttributeNS (writer, (const xmlChar *)prefix,
                                        (const xmlChar *)name, NULL,
                                        (const xmlChar *)content);
}

static inline gint
xml_writer_end_doc (xmlTextWriter *writer)
{
  return xmlTextWriterEndDocument (writer);
}

G_END_DECLS
