package door.bell.ui.home
import android.webkit.WebView
import android.webkit.WebViewClient


class HomeWebViewClient : WebViewClient() {
    override fun shouldOverrideUrlLoading(view: WebView, url: String): Boolean {
        view.loadUrl(url)
        return true
    }
}