#!/usr/bin/env python3
"""
Stress Test Script for URL Shortening Service

This script uses locust to perform load testing on the URL shortening service.
It tests both the shortening (POST) and redirection (GET) endpoints under load.

Usage:
    locust -f stress_test.py --host=http://34.105.32.190/

Then open http://localhost:8089 in your browser to start the test.
"""

import random
import string
from locust import HttpUser, task, between

class URLShortenerUser(HttpUser):
    # Wait between 0.1 and 0.3 seconds between tasks
    wait_time = between(0.1, 0.3)
    
    # Store shortened URLs for later use
    shortened_urls = []
    
    # Target host for the stress test
    host = "http://34.105.32.190/"
    
    def generate_random_url(self):
        """Generate a random URL for testing"""
        domains = ["example.com", "test.org", "demo.net"]
        paths = ["/page", "/article", "/product", "/blog"]
        return f"https://{random.choice(domains)}{random.choice(paths)}/{''.join(random.choices(string.ascii_lowercase + string.digits, k=8))}"

    @task(2)  
    def test_shorten_url(self):
        """Test the URL shortening endpoint"""
        url = self.generate_random_url()
        with self.client.post(
            "shorten",
            data=url,
            headers={"Content-Type": "text/plain"},
            catch_response=True
        ) as response:
            if response.status_code == 200:
                # Store the shortened URL for later use
                self.shortened_urls.append(response.text)
            else:
                response.failure(f"Failed to shorten URL: {response.status_code}")

    @task(10) # Weight of 3 means this task runs 3x more often than others
    def test_redirect(self):
        """Test the URL redirection endpoint"""
        if not self.shortened_urls:
            return
            
        # # Pick a random shortened URL from our list
        # short_url = random.choice(self.shortened_urls)
        # Implement 80/20 rule for URL selection
        if random.random() < 0.8:  # 80% of the time
            # Select from the top 20% of URLs
            top_urls = self.shortened_urls[:max(1, len(self.shortened_urls) // 5)]
            short_url = random.choice(top_urls)
        else:  # 20% of the time
            # Select from the remaining 80% of URLs
            remaining_urls = self.shortened_urls[max(1, len(self.shortened_urls) // 5):]
            short_url = random.choice(remaining_urls)
        
        with self.client.get(
            f"shorten/{short_url}",
            catch_response=True,
            allow_redirects=False  # Don't follow redirects
        ) as response:
            if response.status_code == 302:
                # Verify we got a Location header
                if "Location" not in response.headers:
                    response.failure("No Location header in redirect response")
            else:
                response.failure(f"Expected 302 redirect, got {response.status_code}")

    @task(1)
    def test_invalid_short_url(self):
        """Test behavior with invalid short URLs"""
        # Generate a random invalid short URL
        invalid_url = ''.join(random.choices(string.ascii_letters + string.digits, k=6))
        
        with self.client.get(
            f"shorten/{invalid_url}",
            catch_response=True,
            allow_redirects=False
        ) as response:
            if response.status_code == 404:
                # This is expected for invalid URLs - mark as success
                response.success()
            else:
                response.failure(f"Expected 404 for invalid URL, got {response.status_code}")
