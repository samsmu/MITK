#include "QmitkCommentTextView.h"

QmitkCommentTextView::QmitkCommentTextView()
{
  m_gui.setupUi(this);

  connect(m_gui.done, SIGNAL(clicked()), this, SLOT(done()));
  connect(m_gui.cansel, SIGNAL(clicked()), this, SLOT(cansel()));

  setModal(true);
}

QmitkCommentTextView::~QmitkCommentTextView()
{
}

void QmitkCommentTextView::done()
{
  m_commentText = m_gui.CommentText->toPlainText();

  accept();

  setHidden(true);
  close();
}

void QmitkCommentTextView::cansel()
{
  reject();

  setHidden(true);
  close();
}

QString QmitkCommentTextView::getText()
{
  return m_commentText;
}

int QmitkCommentTextView::show()
{
  m_gui.CommentText->clear();

  int res = exec();

  return res;
}
