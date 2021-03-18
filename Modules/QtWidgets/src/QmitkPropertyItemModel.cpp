/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#include "QmitkPropertyItem.h"
#include "QmitkPropertyItemModel.h"
#include <mitkColorProperty.h>
#include <mitkEnumerationProperty.h>
#include <mitkProperties.h>
#include <mitkIPropertyAliases.h>
#include <mitkIPropertyFilters.h>
#include <mitkRenderingManager.h>
#include <mitkStringProperty.h>
#include <QColor>

#include <usGetModuleContext.h>
#include <usModuleContext.h>
#include <usServiceReference.h>

template <class T>
T* GetPropertyService()
{
  us::ModuleContext* context = us::GetModuleContext();
  us::ServiceReference<T> serviceRef = context->GetServiceReference<T>();

  return serviceRef
    ? context->GetService<T>(serviceRef)
    : NULL;
}

static QColor MitkToQt(const mitk::Color &color)
{
  return QColor(color.GetRed() * 255, color.GetGreen() * 255, color.GetBlue() * 255);
}

static mitk::BaseProperty* GetBaseProperty(const QVariant& data)
{
  return data.isValid()
    ? reinterpret_cast<mitk::BaseProperty*>(data.value<void*>())
    : NULL;
}

static mitk::Color QtToMitk(const QColor &color)
{
  mitk::Color mitkColor;

  mitkColor.SetRed(color.red() / 255.0f);
  mitkColor.SetGreen(color.green() / 255.0f);
  mitkColor.SetBlue(color.blue() / 255.0f);

  return mitkColor;
}

class PropertyEqualTo
{
public:
  PropertyEqualTo(const mitk::BaseProperty* property)
    : m_Property(property)
  {
  }

  bool operator()(const mitk::PropertyList::PropertyMapElementType& pair) const
  {
    return pair.second.GetPointer() == m_Property;
  }

private:
  const mitk::BaseProperty* m_Property;
};

QmitkPropertyItemModel::QmitkPropertyItemModel(QObject* parent)
  : QAbstractItemModel(parent),
    m_ShowAliases(false),
    m_FilterProperties(false),
    m_PropertyAliases(NULL),
    m_PropertyFilters(NULL)
{
  this->CreateRootItem();
}

QmitkPropertyItemModel::~QmitkPropertyItemModel()
{
  this->SetNewPropertyList(NULL);
}

int QmitkPropertyItemModel::columnCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return static_cast<QmitkPropertyItem*>(parent.internalPointer())->GetColumnCount();
  else
    return m_RootItem->GetColumnCount();
}

void QmitkPropertyItemModel::CreateRootItem()
{
    QList<QVariant> rootData;
    rootData << "Property" << "Value";

    m_RootItem.reset(new QmitkPropertyItem(rootData));

    this->beginResetModel();
    this->endResetModel();
}

QVariant QmitkPropertyItemModel::data(const QModelIndex& index, int role) const
{
  if(!index.isValid())
    return QVariant();

  mitk::BaseProperty* property = index.column() == 1
    ? GetBaseProperty(static_cast<QmitkPropertyItem*>(index.internalPointer())->GetData(1))
    : NULL;

  if (role == Qt::DisplayRole)
  {
    if (index.column() == 0)
    {
      return static_cast<QmitkPropertyItem*>(index.internalPointer())->GetData(0);
    }
    else if (index.column() == 1 && property != NULL)
    {
      if (mitk::ColorProperty* colorProperty = dynamic_cast<mitk::ColorProperty*>(property))
        return MitkToQt(colorProperty->GetValue());
      else if (dynamic_cast<mitk::BoolProperty*>(property) == NULL)
        return QString::fromStdString(property->GetValueAsString());
    }
  }
  else if (index.column() == 1 && property != NULL)
  {
    if (role == Qt::CheckStateRole)
    {
      if (mitk::BoolProperty* boolProperty = dynamic_cast<mitk::BoolProperty*>(property))
        return boolProperty->GetValue() ? Qt::Checked : Qt::Unchecked;
    }
    else if (role == Qt::EditRole)
    {
      if (dynamic_cast<mitk::StringProperty*>(property) != NULL)
      {
        return QString::fromStdString(property->GetValueAsString());
      }
      else if (mitk::IntProperty* intProperty = dynamic_cast<mitk::IntProperty*>(property))
      {
        return intProperty->GetValue();
      }
      else if (mitk::FloatProperty* floatProperty = dynamic_cast<mitk::FloatProperty*>(property))
      {
        return floatProperty->GetValue();
      }
      else if (mitk::DoubleProperty* doubleProperty = dynamic_cast<mitk::DoubleProperty*>(property))
      {
        return doubleProperty->GetValue();
      }
      else if (mitk::EnumerationProperty* enumProperty = dynamic_cast<mitk::EnumerationProperty*>(property))
      {
        QStringList values;
        enumProperty->EnumerateIdsContainer([&values](mitk::EnumerationProperty::IdType id, const std::string& value) {
          values << QString::fromStdString(value);
        });
        return values;
      }
      else if (mitk::ColorProperty* colorProperty = dynamic_cast<mitk::ColorProperty*>(property))
      {
        return MitkToQt(colorProperty->GetValue());
      }
    }
    else if (role == mitk::PropertyRole)
    {
      return QVariant::fromValue<void*>(property);
    }
  }

  return QVariant();
}

QModelIndex QmitkPropertyItemModel::FindProperty(const mitk::BaseProperty* property)
{
  if (property == nullptr)
    return QModelIndex();

  if (m_PropertyList.IsExpired())
    return QModelIndex();

  auto propertyMap = m_PropertyList.Lock()->GetMap();
  auto it = std::find_if(propertyMap->begin(), propertyMap->end(), PropertyEqualTo(property));

  if (it == propertyMap->end())
    return QModelIndex();

  QString name = QString::fromStdString(it->first);

  if (!name.contains('.'))
  {
    QModelIndexList item = this->match(index(0, 0), Qt::DisplayRole, name, 1, Qt::MatchExactly);

    if (!item.empty())
      return item[0];
  }
  else
  {
    QStringList names = name.split('.');
    QModelIndexList items =
      this->match(index(0, 0), Qt::DisplayRole, names.last(), -1, Qt::MatchRecursive | Qt::MatchExactly);

    for (auto item : items)
    {
      QModelIndex candidate = item;

      for (int i = names.length() - 1; i != 0; --i)
      {
        QModelIndex parent = item.parent();

        if (parent.parent() == QModelIndex())
        {
          if (parent.data() != names.first())
            break;

          return candidate;
        }

        if (parent.data() != names[i - 1])
          break;

        item = parent;
      }
    }
  }

  return QModelIndex();
}

Qt::ItemFlags QmitkPropertyItemModel::flags(const QModelIndex& index) const
{
  Qt::ItemFlags flags = QAbstractItemModel::flags(index);

  if (index.column() == 1)
  {
    if (index.data(Qt::EditRole).isValid())
      flags |= Qt::ItemIsEditable;

    if (index.data(Qt::CheckStateRole).isValid())
      flags |= Qt::ItemIsUserCheckable;
  }

  return flags;
}

mitk::PropertyList* QmitkPropertyItemModel::GetPropertyList() const
{
  return m_PropertyList.Lock().GetPointer();
}

QVariant QmitkPropertyItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return m_RootItem->GetData(section);

  return QVariant();
}

QModelIndex QmitkPropertyItemModel::index(int row, int column, const QModelIndex& parent) const
{
  if (!this->hasIndex(row, column, parent))
    return QModelIndex();

  QmitkPropertyItem* parentItem = parent.isValid()
    ? static_cast<QmitkPropertyItem*>(parent.internalPointer())
    : m_RootItem.get();

  QmitkPropertyItem* childItem = parentItem->GetChild(row);

  return childItem != NULL
    ? this->createIndex(row, column, childItem)
    : QModelIndex();
}

void QmitkPropertyItemModel::OnPreferencesChanged()
{
    bool updateAliases = m_ShowAliases != (m_PropertyAliases != nullptr);
  bool updateFilters = m_FilterProperties != (m_PropertyFilters != nullptr);

  bool resetPropertyList = false;

  if (updateAliases)
  {
    m_PropertyAliases = m_ShowAliases ? GetPropertyService<mitk::IPropertyAliases>() : nullptr;

    resetPropertyList = !m_PropertyList.IsExpired();
  }

  if (updateFilters)
  {
    m_PropertyFilters = m_FilterProperties ? GetPropertyService<mitk::IPropertyFilters>() : nullptr;

    if (!resetPropertyList)
      resetPropertyList = !m_PropertyList.IsExpired();
  }

  if (resetPropertyList)
    this->SetNewPropertyList(m_PropertyList.Lock());
}

void QmitkPropertyItemModel::OnPropertyDeleted(const itk::Object* /*property*/, const itk::EventObject&)
{
  /*QModelIndex index = this->FindProperty(static_cast<const mitk::BaseProperty*>(property));

  if (index != QModelIndex())
    this->reset();*/
}

void QmitkPropertyItemModel::OnPropertyListModified()
{
  this->SetNewPropertyList(m_PropertyList.Lock());
}

void QmitkPropertyItemModel::OnPropertyListDeleted()
{
  this->CreateRootItem();
}

void QmitkPropertyItemModel::OnPropertyModified(const itk::Object* property, const itk::EventObject&)
{
  QModelIndex index = this->FindProperty(static_cast<const mitk::BaseProperty*>(property));

  if (index != QModelIndex())
    emit dataChanged(index, index);
}

QModelIndex QmitkPropertyItemModel::parent(const QModelIndex& child) const
{
  if (!child.isValid())
    return QModelIndex();

  QmitkPropertyItem* parentItem = static_cast<QmitkPropertyItem*>(child.internalPointer())->GetParent();

  if (parentItem == m_RootItem.get())
    return QModelIndex();

  return this->createIndex(parentItem->GetRow(), 0, parentItem);
}

int QmitkPropertyItemModel::rowCount(const QModelIndex& parent) const
{
  if (parent.column() > 0)
    return 0;

  QmitkPropertyItem *parentItem = parent.isValid()
    ? static_cast<QmitkPropertyItem*>(parent.internalPointer())
    : m_RootItem.get();

  return parentItem->GetChildCount();
}

bool QmitkPropertyItemModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (!index.isValid() || index.column() != 1 || (role != Qt::EditRole && role != Qt::CheckStateRole))
    return false;

  mitk::BaseProperty* property = GetBaseProperty(static_cast<QmitkPropertyItem*>(index.internalPointer())->GetData(1));

  if (property == NULL)
    return false;

  if (mitk::BoolProperty* boolProperty = dynamic_cast<mitk::BoolProperty*>(property))
  {
    boolProperty->SetValue(value.toInt() == Qt::Checked ? true : false);
  }
  else if (mitk::StringProperty* stringProperty = dynamic_cast<mitk::StringProperty*>(property))
  {
    stringProperty->SetValue(value.toString().toStdString());
  }
  else if (mitk::IntProperty* intProperty = dynamic_cast<mitk::IntProperty*>(property))
  {
    intProperty->SetValue(value.toInt());
  }
  else if (mitk::FloatProperty* floatProperty = dynamic_cast<mitk::FloatProperty*>(property))
  {
    floatProperty->SetValue(value.toFloat());
  }
  else if (mitk::DoubleProperty* doubleProperty = dynamic_cast<mitk::DoubleProperty*>(property))
  {
    doubleProperty->SetValue(value.toDouble());
  }
  else if (mitk::EnumerationProperty* enumProperty = dynamic_cast<mitk::EnumerationProperty*>(property))
  {
    std::string selection = value.toString().toStdString();

    if (selection != enumProperty->GetValueAsString() && enumProperty->IsValidEnumerationValue(selection))
      enumProperty->SetValue(selection);
  }
  else if (mitk::ColorProperty* colorProperty = dynamic_cast<mitk::ColorProperty*>(property))
  {
    colorProperty->SetValue(QtToMitk(value.value<QColor>()));
  }

  auto propertyList = m_PropertyList.Lock();

  propertyList->Modified();

  mitk::RenderingManager::GetInstance()->RequestUpdateAll();

  return true;
}

void QmitkPropertyItemModel::SetNewPropertyList(mitk::PropertyList* newPropertyList)
{
  typedef mitk::PropertyList::PropertyMap PropertyMap;

  this->beginResetModel();

  if (!m_PropertyList.IsExpired())
  {
    auto propertyList = m_PropertyList.Lock();

    propertyList->RemoveObserver(m_PropertyListDeletedTag);
    propertyList->RemoveObserver(m_PropertyListModifiedTag);

    const PropertyMap *propertyMap = m_PropertyList.Lock()->GetMap();

    for (PropertyMap::const_iterator propertyIt = propertyMap->begin(); propertyIt != propertyMap->end(); ++propertyIt)
    {
      std::map<std::string, unsigned long>::const_iterator tagIt = m_PropertyModifiedTags.find(propertyIt->first);

      if (tagIt != m_PropertyModifiedTags.end())
        propertyIt->second->RemoveObserver(tagIt->second);

      tagIt = m_PropertyDeletedTags.find(propertyIt->first);

      if (tagIt != m_PropertyDeletedTags.end())
        propertyIt->second->RemoveObserver(tagIt->second);
    }

    m_PropertyModifiedTags.clear();
    m_PropertyDeletedTags.clear();
  }

  m_PropertyList = newPropertyList;

  if (!m_PropertyList.IsExpired())
  {
    auto onPropertyListModified = itk::SimpleMemberCommand<QmitkPropertyItemModel>::New();
    onPropertyListModified->SetCallbackFunction(this, &QmitkPropertyItemModel::OnPropertyListModified);
    m_PropertyListModifiedTag = m_PropertyList.Lock()->AddObserver(itk::ModifiedEvent(), onPropertyListModified);

    auto onPropertyListDeleted = itk::SimpleMemberCommand<QmitkPropertyItemModel>::New();
    onPropertyListDeleted->SetCallbackFunction(this, &QmitkPropertyItemModel::OnPropertyListDeleted);
    m_PropertyListDeletedTag = m_PropertyList.Lock()->AddObserver(itk::DeleteEvent(), onPropertyListDeleted);

    auto modifiedCommand = itk::MemberCommand<QmitkPropertyItemModel>::New();
    modifiedCommand->SetCallbackFunction(this, &QmitkPropertyItemModel::OnPropertyModified);

    const PropertyMap *propertyMap = m_PropertyList.Lock()->GetMap();

    for (PropertyMap::const_iterator it = propertyMap->begin(); it != propertyMap->end(); ++it)
      m_PropertyModifiedTags.insert(
        std::make_pair(it->first, it->second->AddObserver(itk::ModifiedEvent(), modifiedCommand)));
  }

  this->CreateRootItem();

  if (m_PropertyList != nullptr && !m_PropertyList.Lock()->IsEmpty())
  {
    mitk::PropertyList::PropertyMap filteredProperties;
    bool filterProperties = false;

    if (m_PropertyFilters != nullptr &&
        (m_PropertyFilters->HasFilter() || m_PropertyFilters->HasFilter(m_ClassName.toStdString())))
    {
      filteredProperties = m_PropertyFilters->ApplyFilter(*m_PropertyList.Lock()->GetMap(), m_ClassName.toStdString());
      filterProperties = true;
    }

    const mitk::PropertyList::PropertyMap *propertyMap =
      !filterProperties ? m_PropertyList.Lock()->GetMap() : &filteredProperties;

    mitk::PropertyList::PropertyMap::const_iterator end = propertyMap->end();

    for (mitk::PropertyList::PropertyMap::const_iterator iter = propertyMap->begin(); iter != end; ++iter)
    {
      std::vector<std::string> aliases;

      if (m_PropertyAliases != nullptr)
      {
        aliases = m_PropertyAliases->GetAliases(iter->first, m_ClassName.toStdString());

        if (aliases.empty() && !m_ClassName.isEmpty())
          aliases = m_PropertyAliases->GetAliases(iter->first);
      }

      if (aliases.empty())
      {
        QList<QVariant> data;
        data << QString::fromStdString(iter->first)
             << QVariant::fromValue((reinterpret_cast<void *>(iter->second.GetPointer())));

        m_RootItem->AppendChild(new QmitkPropertyItem(data));
      }
      else
      {
        std::vector<std::string>::const_iterator end = aliases.end();
        for (std::vector<std::string>::const_iterator aliasIter = aliases.begin(); aliasIter != end; ++aliasIter)
        {
          QList<QVariant> data;
          data << QString::fromStdString(*aliasIter)
               << QVariant::fromValue((reinterpret_cast<void *>(iter->second.GetPointer())));

          m_RootItem->AppendChild(new QmitkPropertyItem(data));
        }
      }
    }
  }

  this->endResetModel();
}

void QmitkPropertyItemModel::SetPropertyList(mitk::PropertyList* propertyList, const QString& className)
{
if (m_PropertyList != propertyList)
  {
    m_ClassName = className;
    this->SetNewPropertyList(propertyList);
  }
}

void QmitkPropertyItemModel::Update()
{
  this->SetNewPropertyList(m_PropertyList.Lock());
}
