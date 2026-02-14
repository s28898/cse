//
// Created by Bruno Kuś on 08/02/2026.
//

#ifndef MONGO_2_COMMANDTRANSFORMER_H
#define MONGO_2_COMMANDTRANSFORMER_H

#include "StatementTransformer.h"
#include "bsoncxx/document/value.hpp"

struct CommandTransformer
{
    EncryptionService m_encryptionAgent;
    PropertyMetadataStorage m_propertyMetadataStorage;
    
    CommandTransformer(EncryptionService encryptionAgent, PropertyMetadataStorage meta)
      : m_encryptionAgent(encryptionAgent),
        m_propertyMetadataStorage(meta) {}
    
    auto transform_command(const ordered_json &command) -> bsoncxx::document::value
    {
      bsoncxx::builder::basic::document result;
      using bsoncxx::builder::basic::kvp;
      for (const auto &[key, value]: command.items())
      {
        if (key == "documents")
        {
          StatementTransformer st{this->m_encryptionAgent, this->m_propertyMetadataStorage};
          bsoncxx::builder::basic::array out_arr;
          for (const auto& doc : value)
            out_arr.append(st.transform_statement_deep(doc));
          result.append(kvp(key, out_arr.extract()));
          continue;
        }

        if (key == "filter")
        {
          StatementTransformer st{this->m_encryptionAgent, this->m_propertyMetadataStorage};
          result.append(kvp(key, st.transform_statement_deep(value)));
          continue;
        }

        if (key == "updates")
        {
          StatementTransformer st{this->m_encryptionAgent, this->m_propertyMetadataStorage};
          bsoncxx::builder::basic::array out_updates;

          for (const auto& upd : value)
          {
            bsoncxx::builder::basic::document upd_doc;
            for (const auto& [uk, uv] : upd.items())
            {
              if (uk == "q" || uk == "u")
              {
                upd_doc.append(bsoncxx::builder::basic::kvp(uk, st.transform_statement_deep(uv)));
              }
              else
              {
                append_json_kvp(upd_doc, uk, uv);
              }
            }
            out_updates.append(upd_doc.extract());
          }

          result.append(kvp(key, out_updates.extract()));
          continue;
        }

        if (key == "deletes")
        {
          StatementTransformer st{this->m_encryptionAgent, this->m_propertyMetadataStorage};
          bsoncxx::builder::basic::array out_deletes;

          for (const auto& del : value)
          {
            bsoncxx::builder::basic::document del_doc;
            for (const auto& [dk, dv] : del.items())
            {
              if (dk == "q")
              {
                del_doc.append(bsoncxx::builder::basic::kvp(dk, st.transform_statement_deep(dv)));
              }
              else
              {
                append_json_kvp(del_doc, dk, dv);
              }
            }
            out_deletes.append(del_doc.extract());
          }

          result.append(kvp(key, out_deletes.extract()));
          continue;
        }

        append_json_kvp(result, key, value);
      }
      return result.extract();
    }
};


#endif //MONGO_2_COMMANDTRANSFORMER_H
