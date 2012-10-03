#pragma once

enum CompletionPriorities
{
    // these are the "minor priorities", they are added to the base priorities below
    kPriorityUnderscore = 1,
    kPriorityTitleCase  = 2,
    kPriorityHyphen     = 3,

    // these are the base priorities (i.e. most significant)
    kPrioritySinglesAbbreviation            = 1000,
    kPrioritySubsequenceAbbreviation        = 2000,

    kPriorityTagsSinglesAbbreviation        = 3000,
    kPriorityTagsSubsequenceAbbreviation    = 4000,

    kPriorityPrefix                         = 5000,
    kPriorityTagsPrefix                     = 6000,
};
