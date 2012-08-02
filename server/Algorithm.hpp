#pragma once

class Algorithm
{
public:
    static void GlobalInit();

    // no side effects
    static void GenerateDepths(
        const unsigned num_indices,
        const unsigned depth,
        std::vector<std::vector<size_t> >& depth_list);
    // digit_upper_bound is of the form [1, digit_upper_bound]
    // no side effects
    static void ResolveCarries(
        std::vector<size_t>& indices,
        size_t digit_upper_bound);

    static const StringVector* ComputeUnderscoreCached(
        const std::string& word);
    static const StringVector* ComputeTitleCaseCached(
        const std::string& word);
    static void ClearGlobalCache();

    static UnsignedStringPairVectorPtr ComputeUnderscore(
        const std::string& word);
    static UnsignedStringPairVectorPtr ComputeTitleCase(
        const std::string& word);

private:
    static
        boost::unordered_map<size_t, std::vector<std::vector<size_t> > >
        depth_list_cache_;

    static boost::unordered_map<String, StringVector> underscore_cache_;
    static boost::mutex underscore_mutex_;

    static boost::unordered_map<String, StringVector> title_case_cache_;
    static boost::mutex title_case_mutex_;
};
