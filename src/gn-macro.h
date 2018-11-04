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

G_BEGIN_DECLS

#define xml_doc_new(_data, _size)      (xmlReadMemory (_data, _size, "", "UTF-8", \
                                                       XML_PARSE_RECOVER | XML_PARSE_NONET))
#define xml_doc_free(_doc)             (xmlFreeDoc (_doc))
#define xml_doc_get_root_element(_doc) (xmlDocGetRootElement (_doc))

#define xml_reader_new(_data, _size)   (xmlReaderForMemory (_data, _size, "", "UTF-8", \
                                                          XML_PARSE_RECOVER | XML_PARSE_NONET))
#define xml_reader_free(_reader)       (xmlFreeTextReader (_reader))
/* #define xml_reader_get_doc(_reader)    (xmlTextReaderCurrentDoc (_reader)) */

#define xml_reader_read(_reader)           (xmlTextReaderRead (_reader))
#define xml_reader_dup_inner_xml(_reader)  ((gchar *)xmlTextReaderReadInnerXml (_reader))
#define xml_reader_get_node_type(_reader)  (xmlTextReaderNodeType (_reader))
#define xml_reader_get_name(_reader)       ((const gchar *)xmlTextReaderConstName (_reader))
#define xml_reader_get_value(_reader)      ((const gchar *)xmlTextReaderConstValue (_reader))
#define xml_reader_dup_string(_reader)     ((gchar *)xmlTextReaderReadString (_reader))
G_END_DECLS
