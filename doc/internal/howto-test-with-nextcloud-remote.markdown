# Testing with Nextcloud running in docker container

1. Run in shell:
```
docker pull nextcloud
docker run -d -p 8080:80 nextcloud
```

2. Navigate to http://localhost:8080 and create an account

3. Add the "News" app (top-right icon -> "+ apps" -> "News" -> "Enable")

4. Navigate to "News" tab and subscribe to a few feeds (e.g. "+ Subscribe" -> Enter "https://Newsboat.org/news.atom" -> "Subscribe")

5. Configure the nextcloud instance as remote in Newsboat config:
```
urls-source "ocnews"
ocnews-url "http://localhost:8080"
ocnews-login "<nextcloud-username>"
ocnews-password "<nextcloud-password>"
ocnews-flag-star "s"
```
