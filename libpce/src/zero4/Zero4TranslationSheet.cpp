#include "zero4/Zero4TranslationSheet.h"
#include "util/TStringConversion.h"
#include "util/TBufStream.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TBufStream.h"
#include "util/TByte.h"
#include "util/TParse.h"
#include "exception/TGenericException.h"
#include <vector>
#include <map>
#include <cctype>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace BlackT;

namespace Pce {


void Zero4TranslationSheet::addStringEntry(std::string id, std::string content,
                    std::string prefix, std::string suffix,
                    std::string translationPlaceholder,
                    std::string autoNotes,
                    const BlackT::TJson& extraData) {
  Zero4TranslationSheetEntry entry;
  entry.type = Zero4TranslationSheetEntry::type_string;
  entry.stringId = id;
  entry.stringContent = content;
  entry.stringPrefix = prefix;
  entry.stringSuffix = suffix;
  entry.translationPlaceholder = translationPlaceholder;
  entry.notes = autoNotes;
  entry.extraData = extraData;
  
  entries.push_back(entry);
  stringEntryMap[id] = entry;
}

void Zero4TranslationSheet::addCommentEntry(std::string comment) {
  Zero4TranslationSheetEntry entry;
  entry.type = Zero4TranslationSheetEntry::type_comment;
//  entry.stringContent
//    = std::string("//==================================\n")
//      + "// " + comment
//      + std::string("\n//==================================");
  entry.stringContent = comment;
  
  entries.push_back(entry);
}

void Zero4TranslationSheet::addSmallCommentEntry(std::string comment) {
  Zero4TranslationSheetEntry entry;
  entry.type = Zero4TranslationSheetEntry::type_smallComment;
//  entry.stringContent
//    = std::string("//=====\n")
//      + "// " + comment
//      + std::string("\n//=====");
  entry.stringContent = comment;
  
  entries.push_back(entry);
}

void Zero4TranslationSheet::addInlineCommentEntry(std::string comment) {
  Zero4TranslationSheetEntry entry;
  entry.type = Zero4TranslationSheetEntry::type_inlineComment;
  entry.stringContent = comment;
  
  entries.push_back(entry);
}

void Zero4TranslationSheet::addMarkerEntry(std::string content) {
  Zero4TranslationSheetEntry entry;
  entry.type = Zero4TranslationSheetEntry::type_marker;
  entry.stringId = content;
  entries.push_back(entry);
}

void Zero4TranslationSheet::addSetBookEntry(
    std::string id, std::string name) {
  Zero4TranslationSheetEntry entry;
  entry.type = Zero4TranslationSheetEntry::type_setBook;
  entry.stringId = id;
  entry.stringContent = name;
  entries.push_back(entry);
}

void Zero4TranslationSheet::addSetChapterEntry(
    std::string id, std::string name) {
  Zero4TranslationSheetEntry entry;
  entry.type = Zero4TranslationSheetEntry::type_setChapter;
  entry.stringId = id;
  entry.stringContent = name;
  entries.push_back(entry);
}

void Zero4TranslationSheet::addSetPageEntry(
    std::string id, std::string name) {
  Zero4TranslationSheetEntry entry;
  entry.type = Zero4TranslationSheetEntry::type_setPage;
  entry.stringId = id;
  entry.stringContent = name;
  entries.push_back(entry);
}

bool Zero4TranslationSheet::hasStringEntryForId(std::string id) const {
  return (stringEntryMap.find(id) != stringEntryMap.end());
}

const Zero4TranslationSheetEntry& Zero4TranslationSheet
    ::getStringEntryById(std::string id) const {
  return stringEntryMap.at(id);
}

void Zero4TranslationSheet::importCsv(std::string filename) {
  TCsv csv;
  {
    TBufStream ifs;
    ifs.open(filename.c_str());
//    csv.readSjis(ifs);
    csv.readUtf8(ifs);
  }
  
//    std::cerr << "here1" << std::endl;
  for (int j = 0; j < csv.numRows(); j++) {
//    std::cerr << j << std::endl;
    std::string type = csv.cell(0, j);
    if (type.compare("string") == 0) {
      std::string id = csv.cell(1, j);
      std::string prefix = csv.cell(2, j);
      std::string suffix = csv.cell(3, j);
      std::string content = csv.cell(5, j);
      addStringEntry(id, content, prefix, suffix);
    }
  }
//    std::cerr << "here2" << std::endl;
}

void Zero4TranslationSheet::exportCsv(std::string filename,
                      Zero4TranslationSheet::LinkData* linkData) const {
  LinkData newLinkData;
  if (linkData == NULL) {
    // do "dummy" export to get link data
    exportCsv(filename, &newLinkData);
    linkData = &newLinkData;
  }
  
  std::ofstream ofs(filename.c_str(), std::ios_base::binary);

  // map of string content to first cell containing that content,
  // for generating cell equates for duplicate strings
  std::map<std::string, DupeResult> stringMap;
  std::map<std::string, DupeResult> stringMapPrefix;
  std::map<std::string, DupeResult> stringMapSuffix;
  
  std::map<std::string, std::string> boxMap;
  
  Zero4TranslationSheetEntryCollection pendingComments;
  Zero4TranslationSheetEntryCollection pendingInlineComments;
  
  int rowNum = 0;
  bool inDictSection = false;
  for (Zero4TranslationSheetEntryCollection::const_iterator it
        = entries.cbegin();
       it != entries.cend();
       ++it) {
    const Zero4TranslationSheetEntry& entry = *it;
    switch (entry.type) {
    case Zero4TranslationSheetEntry::type_string:
    {
      // col A = type
      ofs << "\"string\"";
      
      // col B = id
      ofs << ",";
      if (!entry.stringId.empty()) {
        ofs << "\"" << entry.stringId << "\"";
      }
      
      // col C = prefix
      ofs << ",";
//      ofs << "\"" << entry.stringPrefix << "\"";
      std::map<std::string, DupeResult>::iterator prefixFindIt
        = stringMapPrefix.end();
      if (!entry.stringPrefix.empty()) {
        std::map<std::string, DupeResult>::iterator findIt
          = stringMapPrefix.find(entry.stringPrefix);
        prefixFindIt = findIt;
        
        if (findIt != stringMapPrefix.end()) {
          ofs << "\"=T(" << findIt->second.rowNum << ")\"";
        }
        else {
          if (!entry.stringPrefix.empty()) {
            ofs << "\"" << entry.stringPrefix << "\"";
          }
//          outputQuotedIfNotEmpty(ofs, entry.stringPrefix);
        
          DupeResult res;
          res.stringId = entry.stringId;
          res.rowNum
            = TCsv::coordinatePosToCellId(prefixContentColNum, rowNum);
          stringMapPrefix[entry.stringPrefix] = res;
        }
      }
      
      // col D = suffix
      ofs << ",";
//      ofs << "\"" << entry.stringSuffix << "\"";
      std::map<std::string, DupeResult>::iterator suffixFindIt
        = stringMapSuffix.end();
      if (!entry.stringSuffix.empty()) {
        std::map<std::string, DupeResult>::iterator findIt
          = stringMapSuffix.find(entry.stringSuffix);
        suffixFindIt = findIt;
        
        if (findIt != stringMapSuffix.end()) {
          ofs << "\"=T(" << findIt->second.rowNum << ")\"";
        }
        else {
          if (!entry.stringSuffix.empty()) {
            ofs << "\"" << entry.stringSuffix << "\"";
          }
//          outputQuotedIfNotEmpty(ofs, entry.stringSuffix);
        
          DupeResult res;
          res.stringId = entry.stringId;
          res.rowNum
            = TCsv::coordinatePosToCellId(suffixContentColNum, rowNum);
          stringMapSuffix[entry.stringSuffix] = res;
        }
      }
      
      // col E = content
      ofs << ",";
      std::map<std::string, DupeResult>::iterator findIt
        = stringMap.find(entry.stringContent);
      // to simplify clutter a bit: pretend null strings are not repeats
      if (findIt != stringMap.end()) {
        if (entry.stringContent.empty()) findIt = stringMap.end();
      }
      
      if (findIt != stringMap.end()) {
//        ofs << "\"=T(" << findIt->second << ")\"";
        if (!entry.stringContent.empty()) {
          ofs << "\"" << entry.stringContent << "\"";
        }
      }
      else {
//        ofs << "\"" << entry.stringContent << "\"";
//        stringMap[entry.stringContent]
//          = TCsv::coordinatePosToCellId(translationContentColNum, rowNum);
        
        // if not linked, produce link data
        if (!linkData->linked) {
          ofs << "\"" << entry.stringContent << "\"";
          
          // add to dictionary if enabled
          if (inDictSection && (!entry.stringContent.empty())) {
            DictEntry dictEntry;
            dictEntry.content = entry.stringContent;
            dictEntry.id = entry.stringId;
            dictEntry.rowNum = rowNum;
//            std::cerr << dictEntry.content << " " << dictEntry.rowNum << std::endl;
            linkData->dictEntries[dictEntry.content] = dictEntry;
          }
        }
        // if linked, check for dictionary substitution
        else {
          // of course, don't link dictionary entries to themselves
          if (inDictSection) {
            if (!entry.stringContent.empty()) {
              ofs << "\"" << entry.stringContent << "\"";
            }
          }
          else {
//            ofs << "\"" << getDictionariedString(*linkData, entry.stringContent)
//                << "\"";
            if (!entry.stringContent.empty()) {
              ofs << "\"" << entry.stringContent << "\"";
            }
          }
        }
        
        DupeResult res;
        res.stringId = entry.stringId;
        res.rowNum
          = TCsv::coordinatePosToCellId(translationContentColNum, rowNum);
        stringMap[entry.stringContent] = res;
      }
      
      // col F = translation placeholder
      ofs << ",";
      if (findIt != stringMap.end()) {
        ofs << "\"=T(" << findIt->second.rowNum << ")\"";
      }
      else {
//        ofs << "\"\"";

        if (!entry.translationPlaceholder.empty()) {
          ofs << "\""
            << entry.translationPlaceholder
            << "\"";
        }
        
        // zenki says the original text is more useful
        // than the placeholders i went to so much trouble to generate :(
//        ofs << "\""
//          << entry.stringContent
////          << getQuoteSubbedString(entry.stringContent)
//          << "\"";
      }
      
      // col G = comment
      ofs << ",";
/*      if (findIt != stringMap.end()) {
//        ofs << "\"DUPE: " << findIt->second << "\"";
//        ofs << "\"DUPE: " << findIt->second.stringId << "\"";
//        ofs << "\"=\"\"DUPE: \"\"&ROW(" << findIt->second.rowNum << ")\"";
//        ofs << "\"NOTE: duplicate\"";
        // this horrible mess is the only way i could find to print an
        // automatically-updating cell reference that works in openoffice...
        ofs << "\"=\"\"DUPE: \"\""
          << "&ADDRESS("
            << "ROW(" << findIt->second.rowNum << "), "
            << "COLUMN(" << findIt->second.rowNum << "), "
            << "4"
          << ")"
          << "\"";
      }
      else*/ {
/*        // box duplication check
        std::map<std::string, std::string>::iterator boxFindIt
          = findBoxDuplicate(boxMap, entry.stringContent);
        if (boxFindIt != boxMap.end()) {
          ofs << "\"PARTIAL DUPE: " << boxFindIt->second << "\"";
        }
        addBoxDuplicates(boxMap, entry.stringContent,
          TCsv::coordinatePosToCellId(translationContentColNum, rowNum)); */
        
        std::ostringstream oss;
        
        // attach any pending inline comments
        if (pendingInlineComments.size() > 0) {
          int count = 0;
          for (auto& item: pendingInlineComments) {
            oss << item.stringContent;
            ++count;
            if (count != pendingInlineComments.size()) oss << std::endl;
          }
          
          pendingInlineComments.clear();
        }
        
        if (!entry.notes.empty()) {
          oss << entry.notes;
        }
        
        if (!oss.str().empty()) {
          ofs << "\"";
          ofs << oss.str();
          ofs << "\"";
        }
      }
      
      // col H = filled check
      ofs << ",";
      if ((findIt == stringMap.end())
          && !entry.stringContent.empty()) {
        ofs << "\"=IF(F" << rowNum+1 << "=I" << rowNum+1
          << ", \"\"UNTRANSLATED\"\", \"\"\"\")\"";
      }
      else if ((findIt != stringMap.end())) {
        // this horrible mess is the only way i could find to print an
        // automatically-updating cell reference that works in openoffice...
        ofs << "\"=\"\"DUPE: \"\""
          << "&ADDRESS("
            << "ROW(" << findIt->second.rowNum << "), "
            << "COLUMN(" << findIt->second.rowNum << "), "
            << "4"
          << ")"
          << "\"";
      }
      
      // col I = filled check value
      ofs << ",";
      if (findIt == stringMap.end()) {
        
        if (!entry.translationPlaceholder.empty()) {
          ofs << "\""
            << entry.translationPlaceholder
            << "\"";
        }
//        ofs << "\""
//          << entry.stringContent
////          << getQuoteSubbedString(entry.stringContent)
//          << "\"";
      }
      
      // col J = dstNotes
      ofs << ",";
      
      // col K = extra data
      ofs << ",";
      {
        // add our own extra data
        
        BlackT::TJson extraData = entry.extraData;
        // HACK: this SHOULD already be an object (or none), but this isn't
        // actually enforced
        extraData.type = BlackT::TJson::type_object;
        
        // add duplication info
        
        if (findIt != stringMap.end()) {
          extraData.content_object["contentIsDupeOf"]
            = BlackT::TJson(findIt->second.stringId);
        }
        
        if (prefixFindIt != stringMapPrefix.end()) {
          extraData.content_object["prefixIsDupeOf"]
            = BlackT::TJson(prefixFindIt->second.stringId);
        }
        
        if (suffixFindIt != stringMapSuffix.end()) {
          extraData.content_object["suffixIsDupeOf"]
            = BlackT::TJson(suffixFindIt->second.stringId);
        }
        
        // attach any pending comments
        if (pendingComments.size() > 0) {
          TJson commentArray;
          commentArray.type = TJson::type_array;
          for (auto& item: pendingComments) {
            // create an object containing comment content and type info
            TJson commentObject;
            commentObject.type = TJson::type_object;
            commentObject.content_object["type"]
              = ((item.type == Zero4TranslationSheetEntry::type_comment)
                  ? TJson("normal") : TJson("small"));
            commentObject.content_object["content"]
              = item.stringContent;
            
            // add to array of comments
            commentArray.content_array.push_back(commentObject);
          }
          
          extraData.content_object["pre_comments"] = commentArray;
          
          // pending comments handled
          pendingComments.clear();
        }
        
        if (extraData.content_object.size() > 0) {
          std::ostringstream oss;
          extraData.write(oss);
          // clean out initial whitespace because it annoys me
          std::string input = oss.str();
          while (!input.empty() && isspace(input[0]))
            input = input.substr(1, std::string::npos);
          
          // escape quotes (csv doesn't use json's backslash notation)
          // (it only just occurs to me now, after years of using this system
          // for preparing scripts, that we should really be doing this for
          // all output strings)
          std::string output;
          for (auto c: input) {
            if (c == '"') output += "\"\"";
            else output += c;
          }
          
          ofs << "\"" << output << "\"";
        }
      }
      
      ofs << std::endl;
      ++rowNum;
    }
      break;
    case Zero4TranslationSheetEntry::type_comment:
    case Zero4TranslationSheetEntry::type_smallComment:
    case Zero4TranslationSheetEntry::type_setBook:
    case Zero4TranslationSheetEntry::type_setChapter:
    case Zero4TranslationSheetEntry::type_setPage:
    {
      // add to pending list so comment can be attached to next string
      switch (entry.type) {
      case Zero4TranslationSheetEntry::type_comment:
      case Zero4TranslationSheetEntry::type_smallComment:
        pendingComments.push_back(entry);
        break;
      default:
        break;
      }
      
      std::string commentStr;
      if ((entry.type == Zero4TranslationSheetEntry::type_smallComment)
          || (entry.type == Zero4TranslationSheetEntry::type_setPage)) {
        commentStr = std::string("//=====\n")
          + "// " + entry.stringContent
          + std::string("\n//=====");
      }
      else {
        commentStr = std::string("//==================================\n")
          + "// " + entry.stringContent
          + std::string("\n//==================================");
      }
      
      // col A = type (blank to ignore)
      switch (entry.type) {
      case Zero4TranslationSheetEntry::type_setBook:
        ofs << "\"book\"";
        break;
      case Zero4TranslationSheetEntry::type_setChapter:
        ofs << "\"chapter\"";
        break;
      case Zero4TranslationSheetEntry::type_setPage:
        ofs << "\"page\"";
        break;
      default:
//        ofs << "\"\"";
        break;
      }
      
      // col B
      ofs << ",";
      switch (entry.type) {
      case Zero4TranslationSheetEntry::type_setBook:
      case Zero4TranslationSheetEntry::type_setChapter:
      case Zero4TranslationSheetEntry::type_setPage:
        ofs << "\"" << entry.stringId << "\"";
        break;
      default:
//        ofs << "\"\"";
        break;
      }
      
      // col C
      ofs << ",";
      switch (entry.type) {
      case Zero4TranslationSheetEntry::type_setBook:
      case Zero4TranslationSheetEntry::type_setChapter:
      case Zero4TranslationSheetEntry::type_setPage:
        ofs << "\"" << entry.stringContent << "\"";
        break;
      default:
//        ofs << "\"\"";
        break;
      }
      
      // col D
      ofs << ",";
//      ofs << "\"\"";
      
      // col E
      ofs << ",";
      if (!commentStr.empty())
        ofs << "\"" << commentStr << "\"";
      
      // col F
      ofs << ",";
//      ofs << "\"\"";
      
      // col G
      ofs << ",";
//      ofs << "\"\"";
      
      // col H
      ofs << ",";
//      ofs << "\"\"";
      
      // col I
      ofs << ",";
//      ofs << "\"\"";
      
      // col J
      ofs << ",";
//      ofs << "\"\"";
      
      // col K
      ofs << ",";
//      ofs << "\"\"";
      
      ofs << std::endl;
      ++rowNum;
    }
      break;
    case Zero4TranslationSheetEntry::type_inlineComment:
    {
      // no output -- these go in the notes field of the next string
      pendingInlineComments.push_back(entry);
    }
      break;
    case Zero4TranslationSheetEntry::type_marker:
    {
      if (entry.stringId.compare("dict_section_start") == 0) {
        inDictSection = true;
      }
      else if (entry.stringId.compare("dict_section_end") == 0) {
        inDictSection = false;
      }
      else {
        throw TGenericException(T_SRCANDLINE,
                                "Zero4TranslationSheet::exportCsv()",
                                "unknown marker: "
                                  + entry.stringId);
      }
    }
      break;
    default:
      throw TGenericException(T_SRCANDLINE,
                              "Zero4TranslationSheet::exportCsv()",
                              "illegal type");
      break;
    }
  }
  
  if (!linkData->linked) {
    linkData->linked = true;
  }
}

int Zero4TranslationSheet::numEntries() const {
  return entries.size();
}

void Zero4TranslationSheet::addBoxDuplicates(
    std::map<std::string, std::string>& boxMap,
    std::string content,
    std::string cell) {
  std::vector<std::string> boxes = splitByBoxes(content);
  for (std::vector<std::string>::iterator it = boxes.begin();
       it != boxes.end();
       ++it) {
    std::map<std::string, std::string>::iterator findIt
      = boxMap.find(*it);
    if (findIt == boxMap.end()) {
      boxMap[*it] = cell;
    }
  }
}

std::map<std::string, std::string>::iterator Zero4TranslationSheet
  ::findBoxDuplicate(
    std::map<std::string, std::string>& boxMap,
    std::string content) {
  std::vector<std::string> boxes = splitByBoxes(content);
  
  for (std::vector<std::string>::iterator it = boxes.begin();
       it != boxes.end();
       ++it) {
    std::map<std::string, std::string>::iterator findIt
      = boxMap.find(*it);
    if (findIt != boxMap.end()) {
      return findIt;
    }
  }
  
  return boxMap.end();
}

std::vector<std::string> Zero4TranslationSheet
  ::splitByBoxes(std::string content) {
  std::vector<std::string> result;
  
  if (content.empty()) return result;
  
  int startPos = 0;
  for (int i = 0; i < content.size() - 1; i++) {
    if ((content[i] == '\\') && (content[i + 1] == 'p')) {
      result.push_back(content.substr(startPos, (i - startPos) + 2));
      startPos = i + 2;
    }
  }
  
  if (startPos < content.size()) {
    result.push_back(content.substr(startPos, content.size() - startPos));
  }
  
  return result;
}

std::string Zero4TranslationSheet::getDictionariedString(LinkData& linkData,
                                         std::string content) {
  std::string result;
  
  result += "=CONCATENATE(";
  
    std::vector<std::string> strings = splitByDictEntries(linkData, content);
    for (unsigned int i = 0; i < strings.size(); i++) {
      result += strings[i];
      if (i != strings.size() - 1) result += ", ";
    }
  
  result += ")";
  
  // my fucking god
  for (int i = 0; i < result.size(); i++) {
    if (result[i] == '\n') {
      result = result.substr(0, i)
        + "\"\", CHAR(10), \"\""
        + result.substr(i + 1, std::string::npos);
    }
  }
  
  return result;
}

std::vector<std::string> Zero4TranslationSheet::splitByDictEntries(
    LinkData& linkData, std::string content) {
  std::map<int, std::string> posToEntryContent;
  
//  std::cerr << content << std::endl;
  for (std::map<std::string, DictEntry>::iterator it
         = linkData.dictEntries.begin();
       it != linkData.dictEntries.end();
       ++it) {
    std::string dictContent = it->second.content;
    // does not handle matching subsequences correctly,
    // but input is such that it doesn't matter
    if (dictContent.size() > content.size()) continue;
    
    for (int i = 0; i < content.size() - dictContent.size(); ) {
//  std::cerr << i << std::endl;
      if (content.substr(i, dictContent.size()).compare(dictContent) == 0) {
        posToEntryContent[i] = dictContent;
        i += dictContent.size();
      }
      else {
        ++i;
      }
    }
  }
  
  std::vector<std::string> result;
  
  // NOTE: the nature of the input is such that overlapping sequences
  // should not be a problem
  int pos = 0;
  int endPos = content.size();
  for (std::map<int, std::string>::iterator it = posToEntryContent.begin();
       it != posToEntryContent.end();
       ++it) {
    // add content from current pos to entry start pos, if any
    int startSectionSize = it->first - pos;
    if (startSectionSize > 0) {
      result.push_back(std::string("\"\"")
        + content.substr(pos, startSectionSize)
        + "\"\"");
    }
    
    // add dictionary sequence
    DictEntry dictEntry = linkData.dictEntries[it->second];
    std::string origCellId = TCsv::coordinatePosToCellId(
      stringContentColNum, dictEntry.rowNum);
    std::string newCellId = TCsv::coordinatePosToCellId(
      translationContentColNum, dictEntry.rowNum);
    // use translation if it exists; otherwise, use original
    std::string newStr = std::string("IF(")
      + newCellId + "=\"\"\"\", "
      + origCellId
      + ", "
      + newCellId
      + ")";
    result.push_back(newStr);
    
    // update current pos to past end of dictionaried term
    pos = it->first + it->second.size();
  }
  
  // add final literal string, if any
  // (entire string if no dictionary sequences)
  if ((endPos - pos) != 0) {
//    result.push_back(content.substr(pos, endPos));
    result.push_back(std::string("\"\"")
      + content.substr(pos, endPos)
      + "\"\"");
  }
  
  return result;
}

std::string Zero4TranslationSheet::getQuoteSubbedString(std::string content) {
  std::vector<std::string> boxStrings = splitByBoxes(content);
  
  std::string result;
  
  for (std::vector<std::string>::iterator it = boxStrings.begin();
       it != boxStrings.end();
       ++it) {
    std::string str = *it;
    TBufStream ifs;
    int quotePos;
    
    // ignore if no trailing box break
    if (str.size() < 2) goto fail;
    if ((str[str.size() - 2] != '\\')
        || (str[str.size() - 1] != 'p')) goto fail;
    
    ifs.writeString(str);
    ifs.seek(0);
    
    // skip initial whitespace
    TParse::skipSpace(ifs);
    
    // skip nametag if it exists
    if (ifs.remaining() < 2) goto fail;
    if (ifs.readu16be() == sjisOpenAngleBracket) {
      while (true) {
        if (ifs.remaining() < 2) goto fail;
        if (ifs.readu16be() == sjisCloseAngleBracket) {
          // check for nametag linebreak
          if (ifs.remaining() < 2) goto fail;
          if (ifs.readu8() != '\\') goto fail;
          if (ifs.readu8() != 'n') goto fail;
          
          break;
        }
      }
    }
    else {
      ifs.seekoff(-2);
    }
    
    // skip whitespace
    TParse::skipSpace(ifs);
    if (ifs.remaining() < 2) goto fail;
    
    // is next symbol a quote?
    if (ifs.readu16be() != sjisOpenQuote) goto fail;
    
    // substitute
//    success:
      quotePos = ifs.tell() - 2;
      str = str.substr(0, quotePos)
            + "{"
            + str.substr(quotePos + 2, str.size() - (quotePos + 2) - 2)
            + "}\\p";
      result += str;
      continue;
    
    fail:
      result += str;
      continue;
      
//    if (str.size() < 2) goto done;
//    
//    done:
//      result += str;
  }
  
  return result;
}

Zero4TranslationSheet::LinkData::LinkData()
  : linked(false) { }


}
