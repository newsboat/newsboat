import scala.io.Source
import java.net.URL
import sys.process._
import scala.io.Source

import scala.concurrent.{Await, Future}
import scala.concurrent.duration._
import scala.concurrent.ExecutionContext.Implicits.global

def isImage(url: String): Boolean = {
    val connection = URL(url).openConnection()
    connection.setDoOutput(false)
    val contentType = connection.getHeaderField("Content-Type")

    contentType match {
        case null => return false
        case contentType: String => contentType.startsWith("image/")
    }
}

@main
def imagePager(kittyString: String, newsboatArticle: String) = {
    val kitty = kittyString.toBoolean

    val urlPattern = """https?://\S+""".r

    val images = Source.fromFile(newsboatArticle)
        .getLines
        .flatMap(urlPattern.findAllIn(_))
        .map(url => Future { (url, isImage(url)) })
        .toList

    val filteredImages = Await.result(Future.sequence(images), Duration.Inf)
        .collect { case (url, true) => url }
        .mkString(" ")

    if (!filteredImages.isEmpty) {
        kitty match {
            case true => {
                //val cols = s"tput cols".!!
                //val lines = s"tput lines".!!
                //val dimensions = s"${cols}x${lines}@0x0".replaceAll("\n", "")

                for (command <- Seq(
                    // This code is mostly from @heussd's kitty-imager-pager.sh bash script
                    // for rendering images with kitty (pull request #1956 on newsboat)
                    "dims=\"$(tput cols)x$(tput lines)@0x0\"",
                    "clear",
                    "kitty +kitten icat --clear",
                    s"kitty +kitten icat --hold --scale-up --place \"$$dims\" $filteredImages",
                    "clear",
                )) {
                    // Pass the command to `bootstrap_image_pager.sh`
                    println(command)
                }
            }
            case false => s"feh $filteredImages".!!
        }
    }
}
