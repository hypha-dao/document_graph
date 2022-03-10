#pragma once

#include <variant>

#include <eosio/multi_index.hpp>
#include <eosio/name.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <eosio/crypto.hpp>
#include <eosio/binary_extension.hpp>

#include <document_graph/content.hpp>
#include <document_graph/content_wrapper.hpp>

namespace hypha
{
    // unused for now, but leaving in the data structure for the future
    struct Certificate
    {
        Certificate() {}
        Certificate(const eosio::name &certifier, const std::string notes) : certifier{certifier}, notes{notes} {}

        eosio::name certifier;
        std::string notes;
        eosio::time_point certification_date = eosio::current_time_point();

        EOSLIB_SERIALIZE(Certificate, (certifier)(notes)(certification_date))
    };

    struct Document
    {
    public:
        Document();

        // these constructors populate a Document instance and emplace
        Document(eosio::name contract, eosio::name creator, ContentGroups contentGroups);
        Document(eosio::name contract, eosio::name creator, ContentGroup contentGroup);
        Document(eosio::name contract, eosio::name creator, Content content);
        Document(eosio::name contract, eosio::name creator, const std::string &label, const Content::FlexValue &value);

        // this constructor reads the hash from the table and populates the object from storage
        Document(eosio::name contract, uint64_t id);
        ~Document();

        void emplace();
        
        /**
         * @brief Updates the document in the multi_index table with the given content groups
         * 
         * @param updatedData
         */
        void update();

        static bool exists(eosio::name contract, uint64_t _id);

        // certificates are not yet used
        void certify(const eosio::name &certifier, const std::string &notes);

        // static helpers
        static ContentGroups rollup(ContentGroup contentGroup);
        static ContentGroups rollup(Content content);
        static void insertOrReplace(ContentGroup &contentGroup, Content &newContent);

        static Document merge(Document original, Document &deltas);

        // vanilla accessors
        ContentWrapper getContentWrapper() { return ContentWrapper(content_groups); }
        ContentGroups &getContentGroups() { return content_groups; }
        const ContentGroups &getContentGroups() const { return content_groups; }
        const eosio::time_point &getCreated() const { return created_date; }
        const eosio::name &getCreator() const { return creator; }
        const eosio::name &getContract() const { return contract; }
        eosio::time_point getUpdatedDate() const { return updated_date.value_or(); }

        // This has to be public in order to be reachable by the abi-generator macro
        // indexes for table
        uint64_t by_created() const { return created_date.sec_since_epoch(); }
        uint64_t by_creator() const { return creator.value; }
        
        inline uint64_t getID() { return id; }
    //private:
        // members, with names as serialized - these must be public for EOSIO tables
        std::uint64_t id;
        eosio::name creator;
        ContentGroups content_groups;
        std::vector<Certificate> certificates;
        eosio::time_point created_date;
        eosio::name contract;
        eosio::binary_extension<eosio::time_point> updated_date;

        // toString iterates through all content, all levels, concatenating all values
        // the resulting string is used for fingerprinting and hashing
        const std::string toString();
        static const std::string toString(const ContentGroups &contentGroups);
        static const std::string toString(const ContentGroup &contentGroup);

        EOSLIB_SERIALIZE(Document, (id)(creator)(content_groups)(certificates)(created_date)(contract)(updated_date))

    public:
        // for unknown reason, primary_key() must be public
        uint64_t primary_key() const { return id; }

        typedef eosio::multi_index<eosio::name("documents"), Document,
                                   eosio::indexed_by<eosio::name("bycreator"), eosio::const_mem_fun<Document, uint64_t, &Document::by_creator>>,
                                   eosio::indexed_by<eosio::name("bycreated"), eosio::const_mem_fun<Document, uint64_t, &Document::by_created>>>
            document_table;
    };

} // namespace hypha