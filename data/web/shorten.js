function shorten_url() {
    const long_url = document.getElementById('long_url').value;
    const error = document.getElementById('error');
    const result = document.getElementById('result');
    const loading = document.getElementById('loading');

    // Reset UI
    error.classList.remove('show');
    result.classList.remove('show');
    loading.classList.add('show');

    // Validate URL
    if (!is_valid_url(long_url)) {
        error.textContent = 'Please enter a valid URL';
        error.classList.add('show');
        loading.classList.remove('show');
        return;
    }

    // Send request to server
    fetch('/shorten', {
        method: 'POST',
        body: long_url
    })
    .then(response => {
        if (!response.ok) {
            throw new Error('Network response was not ok');
        }
        return response.text();
    })
    .then(short_url => {
        loading.classList.remove('show');
        document.getElementById('short_url').textContent = window.location.origin + '/shorten/' + short_url;
        result.classList.add('show');
    })
    .catch(err => {
        loading.classList.remove('show');
        error.textContent = 'An error occurred. Please try again.';
        error.classList.add('show');
    });
}

function copy_to_clipboard() {
    const short_url = document.getElementById('short_url').textContent;
    const copy_btn = document.querySelector('.copy-btn');
    const original_text = copy_btn.textContent;

    // Try using the Clipboard API first
    if (navigator.clipboard && navigator.clipboard.writeText) {
        navigator.clipboard.writeText(short_url)
            .then(() => {
                copy_btn.textContent = 'Copied!';
                setTimeout(() => {
                    copy_btn.textContent = original_text;
                }, 2000);
            })
            .catch(err => {
                console.error('Failed to copy text: ', err);
                fallback_copy_to_clipboard(short_url, copy_btn, original_text);
            });
    } else {
        // Fallback for browsers that don't support Clipboard API
        fallback_copy_to_clipboard(short_url, copy_btn, original_text);
    }
}

function fallback_copy_to_clipboard(text, button, original_text) {
    // Create a temporary textarea element
    const text_area = document.createElement('textarea');
    text_area.value = text;
    
    // Make the textarea out of viewport
    text_area.style.position = 'fixed';
    text_area.style.left = '-999999px';
    text_area.style.top = '-999999px';
    document.body.appendChild(text_area);
    
    // Select and copy the text
    text_area.focus();
    text_area.select();
    
    try {
        const successful = document.execCommand('copy');
        if (successful) {
            button.textContent = 'Copied!';
            setTimeout(() => {
                button.textContent = original_text;
            }, 2000);
        } else {
            console.error('Failed to copy text');
        }
    } catch (err) {
        console.error('Failed to copy text: ', err);
    }
    
    // Clean up
    document.body.removeChild(text_area);
}

function is_valid_url(string) {
    try {
        new URL(string);
        return true;
    } catch (_) {
        return false;
    }
}

// Add event listener for Enter key
document.getElementById('long_url').addEventListener('keypress', function(event) {
    if (event.key === 'Enter') {
        event.preventDefault(); // Prevent form submission
        shorten_url();
    }
}); 