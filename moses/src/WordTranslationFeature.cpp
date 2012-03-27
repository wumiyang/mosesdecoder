#include <sstream>
#include "WordTranslationFeature.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "ScoreComponentCollection.h"

namespace Moses {

using namespace std;

bool WordTranslationFeature::Load(const std::string &filePathSource, const std::string &filePathTarget) 
{
  // restricted source word vocabulary
  ifstream inFileSource(filePathSource.c_str());
  if (!inFileSource)
  {
      cerr << "could not open file " << filePathSource << endl;
      return false;
  }

  std::string line;
  while (getline(inFileSource, line)) {
    m_vocabSource.insert(line);
  }

  inFileSource.close();

  // restricted target word vocabulary
  ifstream inFileTarget(filePathTarget.c_str());
  if (!inFileTarget)
  {
      cerr << "could not open file " << filePathTarget << endl;
      return false;
  }

  while (getline(inFileTarget, line)) {
    m_vocabTarget.insert(line);
  }

  inFileTarget.close();

  m_unrestricted = false;
  return true;
}

void WordTranslationFeature::InitializeForInput( Sentence const& in )
{
  m_local.reset(new ThreadLocalStorage);
  m_local->input = &in;
}

//void WordTranslationFeature::Evaluate(const TargetPhrase& targetPhrase, ScoreComponentCollection* accumulator) const
FFState* WordTranslationFeature::Evaluate(const Hypothesis& cur_hypo, const FFState* prev_state, ScoreComponentCollection* accumulator) const
{
	const Sentence& input = *(m_local->input);
	const TargetPhrase& targetPhrase = cur_hypo.GetCurrTargetPhrase();
  const AlignmentInfo &alignment = targetPhrase.GetAlignmentInfo();

  // process aligned words
  for (AlignmentInfo::const_iterator alignmentPoint = alignment.begin(); alignmentPoint != alignment.end(); alignmentPoint++) {
    // look up words
  	const Phrase& sourcePhrase = targetPhrase.GetSourcePhrase();
  	int sourceIndex = alignmentPoint->first;
  	int targetIndex = alignmentPoint->second;
    const string &sourceWord = sourcePhrase.GetWord(sourceIndex).GetFactor(m_factorTypeSource)->GetString();
    const string &targetWord = targetPhrase.GetWord(targetIndex).GetFactor(m_factorTypeTarget)->GetString();
    if (m_ignorePunctuation) {
      // check if source or target are punctuation                                                                             
      char firstChar = sourceWord.at(0);
      CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
      if(charIterator != m_punctuationHash.end())
	continue;
      firstChar = targetWord.at(0);
      charIterator = m_punctuationHash.find( firstChar );
      if(charIterator != m_punctuationHash.end())
        continue;
    }

    bool sourceExists = false;
    bool targetExists = false;
    if (!m_unrestricted) {
    	sourceExists = m_vocabSource.find( sourceWord ) != m_vocabSource.end();
    	targetExists = m_vocabTarget.find( targetWord ) != m_vocabTarget.end();
    }
    // no feature if both words are not in restricted vocabularies
    if (m_unrestricted || (sourceExists && targetExists)) {
    	if (m_simple) {
    		// construct feature name
    		stringstream featureName;
    		featureName << "wt_";
    		//featureName << ((sourceExists||m_unrestricted) ? sourceWord : "OTHER");
    		featureName << sourceWord;
    		featureName << "~";
    		//featureName << ((targetExists||m_unrestricted) ? targetWord : "OTHER");
    		featureName << targetWord;
    		accumulator->SparsePlusEquals(featureName.str(), 1);
    	}
    	if (m_sourceContext) {
    		size_t globalSourceIndex = cur_hypo.GetCurrSourceWordsRange().GetStartPos() + sourceIndex;
    		if (globalSourceIndex == 0) {
    			// add <s> trigger feature for source
    			stringstream feature;
    			feature << "wt_";
    			feature << "<s>,";
    			feature << sourceWord;
    			feature << "~";
    			feature << targetWord;
    			accumulator->SparsePlusEquals(feature.str(), 1);
    		}

    		// range over source words to get context
    		for(size_t contextIndex = 0; contextIndex < input.GetSize(); contextIndex++ ) {
    			if (contextIndex == globalSourceIndex) continue;
    			string sourceTrigger = input.GetWord(contextIndex).GetFactor(m_factorTypeSource)->GetString();
    		  	if (m_ignorePunctuation) {
    		  		// check if trigger is punctuation
    		  		char firstChar = sourceTrigger.at(0);
    		  		CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
    		  		if(charIterator != m_punctuationHash.end())
    		  			continue;
    		  	}
    			
    			bool sourceTriggerExists = false;
    			if (!m_unrestricted)
    				sourceTriggerExists = m_vocabSource.find( sourceTrigger ) != m_vocabSource.end();

    			if (m_unrestricted || sourceTriggerExists) {
	    			stringstream feature;
	    			feature << "wt_";
	    			if (contextIndex < globalSourceIndex) {
	    				feature << sourceTrigger;
	    				feature << ",";
	    				feature << sourceWord;
	    			}
	    			else {
	    				feature << sourceWord;
	    				feature << ",";
	    				feature << sourceTrigger;
	    			}
	    			feature << "~";
	    			feature << targetWord;
	    			accumulator->SparsePlusEquals(feature.str(), 1);
    			}
    		}
    	}
    	if (m_targetContext) {
    		size_t globalTargetIndex = cur_hypo.GetCurrTargetWordsRange().GetStartPos() + targetIndex;
    		if (globalTargetIndex == 0) {
    			// add <s> trigger feature for source
    			stringstream feature;
    			feature << "wt_";
    			feature << sourceWord;
    			feature << "~";
    			feature << "<s>,";
    			feature << targetWord;
    			accumulator->SparsePlusEquals(feature.str(), 1);    	   
    		}

    		// range over target words (up to current position) to get context
    		for(size_t contextIndex = 0; contextIndex < globalTargetIndex; contextIndex++ ) {
    			string targetTrigger = cur_hypo.GetWord(contextIndex).GetFactor(m_factorTypeTarget)->GetString();
    		  	if (m_ignorePunctuation) {
    		  		// check if trigger is punctuation
    		  		char firstChar = targetTrigger.at(0);
    		  		CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
    		  		if(charIterator != m_punctuationHash.end())
    		  			continue;
    		  	}
    			
    			bool targetTriggerExists = false;
    			if (!m_unrestricted)
    				targetTriggerExists = m_vocabTarget.find( targetTrigger ) != m_vocabTarget.end();

    			if (m_unrestricted || targetTriggerExists) {
    				stringstream feature;
    				feature << "wt_";
    				feature << sourceWord;
    				feature << "~";
	    			feature << targetTrigger;
	    			feature << ",";
	    			feature << targetWord;
	    			accumulator->SparsePlusEquals(feature.str(), 1);
    			}
    		}
    	}
    }
  }

  return new DummyState();
}

}