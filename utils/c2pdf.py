# convert linux style c source file to pdf

import argparse
from reportlab.lib.pagesizes import letter
from reportlab.platypus import SimpleDocTemplate, Preformatted, Spacer
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.utils import simpleSplit
from reportlab.pdfgen import canvas

class NumberedCanvas(canvas.Canvas):
    def __init__(self, *args, **kwargs):
        canvas.Canvas.__init__(self, *args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_page_number(num_pages)
            canvas.Canvas.showPage(self)
        canvas.Canvas.save(self)

    def draw_page_number(self, page_count):
        self.setFont("Helvetica", 9)
        self.drawRightString(letter[0] - 30, 30, f"Page {self._pageNumber} of {page_count}")

def text_to_pdf(input_file, output_file):
    # Create a PDF document
    doc = SimpleDocTemplate(output_file, pagesize=letter)
    
    # Create a custom style for the code
    styles = getSampleStyleSheet()
    code_style = ParagraphStyle(
        'CodeStyle',
        parent=styles['Code'],
        fontName='Courier',
        fontSize=8,
        leading=9.6,  # 1.2 times the font size for good readability
    )

    # Read the input file and process its content
    with open(input_file, 'r') as file:
        content = file.read()
    
    # Replace tabs with 8 spaces
    content = content.replace('\t', ' ' * 8)
    
    # Create a list to hold the PDF elements
    elements = []
    
    # Split content into pages to avoid overflow
    page_width, page_height = letter
    lines = content.split('\n')
    current_page = []
    
    for line in lines:
        current_page.append(line)
        if len(current_page) * code_style.leading > page_height - 72:  # 72 points margin
            elements.append(Preformatted('\n'.join(current_page), code_style))
            elements.append(Spacer(1, 12))
            current_page = []
    
    if current_page:
        elements.append(Preformatted('\n'.join(current_page), code_style))
    
    # Build the PDF
    doc.build(elements, canvasmaker=NumberedCanvas)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert a text file to PDF')
    parser.add_argument('input_file', help='Input text file')
    parser.add_argument('output_file', help='Output PDF file')
    args = parser.parse_args()

    text_to_pdf(args.input_file, args.output_file)
    print(f"PDF created: {args.output_file}")
