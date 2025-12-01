# Stack Exchange Search

We use the Stack Exchange API to search for information curated by a given user. This script is helpful for learning about a given topic from a relevant subject matter expert (SME).

No API key is necessary to use the Stack Exchange API within [300 requests per IP per day](https://meta.stackoverflow.com/a/356420/5792097). This limit is more than enough for our purpose of collecting lots of data about a few event types because the API lets us return many entries per response.

Many Microsoft employees have Stack Exchange accounts and some regularly share information there. The information could be useful for learning about how Microsoft products, such as Microsoft Windows, work and especially why they work the way they do.

Usage: `./get-stack-exchange-user-contributions.py <USER_ID> <SITE>`

Example: `./get-stack-exchange-user-contributions.py 5792097 stackoverflow`
