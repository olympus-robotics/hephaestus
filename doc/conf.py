import os

project = 'Hephaestus'
author = 'Hephaestus Contributors'
copyright = f'2025, Hephaestus Contributors'
release = '0.0.1'
exclude_patterns = [
    '**/*bazel*',
    'requirements.*',
]
extensions = ['sphinx.ext.todo', 'sphinx.ext.autodoc', 'breathe', 'sphinxcontrib.mermaid']
pygments_style = 'sphinx'

todo_include_todos = True

breathe_projects ={"heph":  os.path.abspath("doxygen/xml")}
breathe_projects_source = {}
breathe_default_project = "heph"

html_theme = 'sphinx_book_theme'

# Theme options are theme-specific and customize the look and feel of a theme
# further.  For a list of options available for each theme, see the
# documentation.
html_theme_options = {
    "home_page_in_toc": True,
    "repository_url": "https://github.com/olympus-robotics/hephaestus",
    "use_repository_button": True,
    "use_issues_button": True
}
