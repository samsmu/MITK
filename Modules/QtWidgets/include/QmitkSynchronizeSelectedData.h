#pragma once

#include <boost/thread/recursive_mutex.hpp>
#include <boost/function.hpp>
#include <boost/signals2.hpp>
#include <boost/noncopyable.hpp>

#include <mitkDataNode.h>

#include <MitkQtWidgetsExports.h>

enum class SynchronizeEventType
{
  IMAGE_CHANGE,
  SEGMENTATION_CHANGE
};

typedef boost::signals2::signal<void(const mitk::DataNode*)> SlotType;
typedef boost::signals2::signal<void(const std::vector<mitk::DataNode*>&)> SlotListType;

class MITKQTWIDGETS_EXPORT QmitkSynchronizeSelectedData : boost::noncopyable
{

public:

  static boost::signals2::connection addObserver(SynchronizeEventType type, const SlotType::slot_type& func);
  static boost::signals2::connection addImageListObserver(const SlotListType::slot_type& func);

  static void emitImageChange(mitk::DataNode* node);
  static void emitSegmentationChange(mitk::DataNode* node);

  // Expected that list is sorted by selection order
  static void emitImageListChange(std::vector<mitk::DataNode*>& nodes);

private:

  static boost::recursive_mutex m_mutex;
  
  static SlotType m_imageChange;
  static SlotType m_segmentationChange;
  static SlotListType m_imageListChange;
};
