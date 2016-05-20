
# Plug-ins must be ordered according to their dependencies

set(MITK_PLUGINS

  org.blueberry.core.runtime:ON
  org.blueberry.core.expressions:ON
  org.blueberry.core.commands:ON
  org.blueberry.ui.qt:ON
  org.blueberry.ui.qt.help:ON

  org.mitk.core.services:ON
  org.mitk.gui.common:ON
  org.mitk.core.ext:ON
  org.mitk.gui.qt.application:ON
  org.mitk.gui.qt.ext:ON
  org.mitk.gui.qt.extapplication:ON
)
