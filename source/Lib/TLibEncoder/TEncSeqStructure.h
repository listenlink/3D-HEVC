

#if !defined(AFX_TEncSeqStructure_H__268768B8_4D1D_484A_904E_586985833BAC__INCLUDED_)
#define AFX_TEncSeqStructure_H__268768B8_4D1D_484A_904E_586985833BAC__INCLUDED_

#include <cctype>
#include "../TLibCommon/CommonDef.h"
#include <vector>

//SB
#include <map>


class FrameDescriptor
{
public:
  ErrVal initFrameDescriptor( const std::string&  rcString, UInt &ruiIncrement, std::map<UInt, UInt>& cColDirTracker);

  const std::vector<int>& getAllowedRelativeRefPocs( RefPicList eRefPicList, UInt uiViewIdx ) const {
    assert( eRefPicList == REF_PIC_LIST_0 || eRefPicList == REF_PIC_LIST_1 );
    assert( m_aaiAllowedRelativeRefPocsL0.size() == m_aaiAllowedRelativeRefPocsL1.size() );
    if( m_aaiAllowedRelativeRefPocsL0.size() <= uiViewIdx )
    {
      return eRefPicList == REF_PIC_LIST_0 ? m_aaiAllowedRelativeRefPocsL0.back() : m_aaiAllowedRelativeRefPocsL1.back();
    }
    return eRefPicList == REF_PIC_LIST_0 ? m_aaiAllowedRelativeRefPocsL0[uiViewIdx]: m_aaiAllowedRelativeRefPocsL1[uiViewIdx];
  }

  const std::vector<int>& getAllowedReferenceViewIdx( RefPicList eRefPicList, UInt uiViewIdx  ) const {
    assert( eRefPicList == REF_PIC_LIST_0 || eRefPicList == REF_PIC_LIST_1 );
    assert( m_aaiAllowedReferenceViewIdxL0.size() == m_aaiAllowedReferenceViewIdxL1.size() );
    if( m_aaiAllowedReferenceViewIdxL0.size() <= uiViewIdx )
    {
      return eRefPicList == REF_PIC_LIST_0 ? m_aaiAllowedReferenceViewIdxL0.back() : m_aaiAllowedReferenceViewIdxL1.back();
    }
    return eRefPicList == REF_PIC_LIST_0 ? m_aaiAllowedReferenceViewIdxL0[uiViewIdx]: m_aaiAllowedReferenceViewIdxL1[uiViewIdx];
  }

  UInt getTEncSeqStructureLayer(UInt uiViewIdx)  const {
    if (m_auiLayer.size() <= uiViewIdx)
    {
      return m_auiLayer.back();
    }
    return m_auiLayer[uiViewIdx];
  }

  bool getStoreForRef(UInt uiViewIdx)            const {
    if (m_abStoreForRef.size() <= uiViewIdx)
    {
      return m_abStoreForRef.back();
    }
    return m_abStoreForRef[uiViewIdx];
  }
  bool isIdr(UInt uiViewIdx)                     const {
    if (m_abIsIDR.size() <= uiViewIdx)
    {
      return m_abIsIDR.back();
    }
    return m_abIsIDR[uiViewIdx];
  }
  SliceType getSliceType(UInt uiViewIdx)        const {
    if (m_aeSliceType.size() <= uiViewIdx)
    {
      return m_aeSliceType.back();
    }
    return m_aeSliceType[uiViewIdx];
  }

  UInt getColDir()  const { return m_uiColDir ; }

  void addLayerOffset( Int iLayerOffset ) {
     for(Int i = 0; i<m_auiLayer.size(); i++)
     {
       m_auiLayer[i] += iLayerOffset ;
     }
  }

private:
  std::vector<bool> m_abStoreForRef;
  std::vector<bool> m_abIsIDR;
  std::vector<UInt> m_auiLayer;
  std::vector<SliceType>  m_aeSliceType;
  std::vector< std::vector<int> > m_aaiAllowedRelativeRefPocsL0;
  std::vector< std::vector<int> > m_aaiAllowedRelativeRefPocsL1;
  std::vector< std::vector<int> > m_aaiAllowedReferenceViewIdxL0;
  std::vector< std::vector<int> > m_aaiAllowedReferenceViewIdxL1;
  UInt m_uiColDir ;

};
class TEncSeqStructure
{
private:
  class SequencePart
  {
    DISABLE_DEFAULT_CPYCTOR_AND_ASOP( SequencePart );
  protected:
    SequencePart() : m_uiNumberOfRepetitions( 0 ) {}

  public:
    virtual ~SequencePart() {}

    virtual bool isLeafNode() const = 0;
    virtual SequencePart* getChildNode( UInt64 ui ) const { TOT( 1 ); return NULL; }
    virtual const FrameDescriptor* getFrameDescriptor( UInt64 ui ) const { TOT( 1 ); return NULL; }

    UInt64 getSize() const { TOT( isInfinitelyLong() ); return UInt64( m_uiNumberOfRepetitions ) * xGetSize(); }
    UInt64 getIncrement( UInt64 ui ) const { return ui - UInt64( xGetIdx( ui ) ) + UInt64( xGetIncrement( ui ) ); }
//    bool isInfinitelyLong() const { return m_uiNumberOfRepetitions == TypeLimits<UInt>::m_iMax; }
    bool isInfinitelyLong() const { return m_uiNumberOfRepetitions == MAX_UINT; }

    virtual UInt64 findIncrement( UInt64 uiIncrement, bool &rbSuccess ) const { TOT( 1 ); return 0; }

  protected:
    UInt xGetIdx( UInt64 ui ) const { return UInt( ui % xGetSize() ); }
    void xSetNumRep( UInt ui )
    {
      TOT( m_uiNumberOfRepetitions );
      TOF( ui );
      m_uiNumberOfRepetitions = ui;
    }

  private:
    virtual UInt   xGetIncrement( UInt64 ui ) const { TOT( 1 ); return 0; }
    virtual UInt64 xGetSize() const = 0;

    UInt m_uiNumberOfRepetitions; //GT: *n5{...}
  };


  class FrameSequencePart : public SequencePart //GT: Sequence of Frames without repetitions
  {
  public:
    FrameSequencePart( const std::string& rcString );
    virtual ~FrameSequencePart() {}

    UInt64 xGetSize() const { return UInt64( m_acFrameDescriptors.size() ); }
    bool isLeafNode() const { return true; }
    const FrameDescriptor* getFrameDescriptor( UInt64 ui ) const { return &m_acFrameDescriptors[xGetIdx( ui )]; }
    UInt xGetIncrement( UInt64 ui ) const { return m_auiIncrements[xGetIdx( ui )]; }
    virtual UInt64 findIncrement( UInt64 uiIncrement, bool &rbSuccess ) const;

  private:
    ErrVal xCheck();

    std::vector<FrameDescriptor> m_acFrameDescriptors; //GT: one descriptor for each frame B3L2(-4,+3)
    std::vector<UInt> m_auiIncrements;                 //GT: same size as frame descriptor, stores increments  P7L1(-8)B3L2(-4,+3)b0L4(-2,+2,+6) -> 1 3 0

    std::map<UInt,UInt> m_cColDirTracker ;
  };


  class GeneralSequencePart : public SequencePart  //GT: Sequence of FrameSequences
  {
  public:
    GeneralSequencePart( const std::string& rcString );
    virtual ~GeneralSequencePart() {}

    UInt64 xGetSize() const { return UInt64( m_apcSequenceParts.size() ); }
    bool isLeafNode() const { return false; }
    SequencePart* getChildNode( UInt64 ui ) const { return m_apcSequenceParts[xGetIdx( ui )]; }

  private:
    AutoDeletePtrVector<SequencePart> m_apcSequenceParts;
  };

public:
  TEncSeqStructure();
  virtual ~TEncSeqStructure();

  ErrVal init( const std::string& rcString );
  UInt getMaxAbsPocDiff( const UInt uiNumberOfFrames );
  UInt getDecodedPictureBufferSize( const UInt uiNumberOfFrames );
  class Iterator
  {
  private:
    class SequencePartWithPos
    {
    public:
      const SequencePart *m_pcSeqPart;
      UInt64 m_uiCurrPos;
    };

  public:
    Iterator() : m_cBasePoc( 0 ) {}
    virtual ~Iterator() {}

    Iterator( const TEncSeqStructure &r, PicOrderCnt cLayerChangeStartPoc, int iLayerOffset );

    FrameDescriptor getFrameDescriptor() const
    {
      if( m_iLayerOffset != 0 && getPoc() >= m_cLayerChangeStartPoc )
      {
        FrameDescriptor cModified = *xGetCurr().m_pcSeqPart->getFrameDescriptor( xGetCurr().m_uiCurrPos );
        cModified.addLayerOffset ( m_iLayerOffset );
        return cModified;
      }
      return *xGetCurr().m_pcSeqPart->getFrameDescriptor( xGetCurr().m_uiCurrPos );
    }

    PicOrderCnt getPoc() const { return m_cBasePoc + xGetCurr().m_pcSeqPart->getIncrement( xGetCurr().m_uiCurrPos ); }

    Iterator& operator++();
    Iterator& traverseByPocDiff( Int64 iPocDiff );
    Iterator getIterByPocDiff( Int64 iPocDiff ) const;

  private:
    SequencePartWithPos&       xGetCurr()       { return m_acSeqPartPath.back(); }
    const SequencePartWithPos& xGetCurr() const { return m_acSeqPartPath.back(); }

    void xGoToPreviousFrameSequencePart();
    void xGoToNextFrameSequencePart();

    void xGoToLeaf( bool bGoToRightmostLeaf );

    std::vector<SequencePartWithPos> m_acSeqPartPath;
    PicOrderCnt m_cBasePoc;
    PicOrderCnt m_cLayerChangeStartPoc;
    int m_iLayerOffset;
  };

private:
  SequencePart *m_pcSequencePart;
};




#endif // !defined(AFX_TEncSeqStructure_H__268768B8_4D1D_484A_904E_586985833BAC__INCLUDED_)
