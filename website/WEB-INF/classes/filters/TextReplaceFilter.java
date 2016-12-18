package filters;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Enumeration;

import javax.servlet.Filter;
import javax.servlet.FilterChain;
import javax.servlet.FilterConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletOutputStream;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.WriteListener;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpServletResponseWrapper;

/**
 * A filter to replace place-holders in a serviced resource with configured
 * values
 * 
 * @author Michael Neuweiler
 *
 */
public class TextReplaceFilter implements Filter {

	private String[] replaceFrom = new String[0];
	private String[] replaceTo = new String[0];

	@Override
	public void destroy() {
		replaceFrom = new String[0];
		replaceTo = new String[0];
	}

	@Override
	public void doFilter(ServletRequest request, ServletResponse response, FilterChain chain)
			throws IOException, ServletException {
		ByteResponseWrapper wrapper = new ByteResponseWrapper((HttpServletResponse) response);
		chain.doFilter(request, wrapper);
		String out = new String(wrapper.getBytes());
		for (int i = 0; i < replaceFrom.length; i++) {
			out = out.replaceAll(replaceFrom[i], replaceTo[i]);
		}
		response.setContentLength(out.length());
		response.getWriter().print(out);
	}

	@Override
	public void init(FilterConfig filterConfig) throws ServletException {
		Enumeration<String> e = filterConfig.getInitParameterNames();
		ArrayList<String> params = new ArrayList<>();
		ArrayList<String> values = new ArrayList<>();
		while (e.hasMoreElements()) {
			String param = e.nextElement();
			params.add(param);
			values.add(filterConfig.getInitParameter(param));
		}
		replaceFrom = params.toArray(new String[0]);
		replaceTo = values.toArray(new String[0]);
	}

	static class ByteResponseWrapper extends HttpServletResponseWrapper {
		private PrintWriter writer;
		private ByteOutputStream output;

		public byte[] getBytes() {
			writer.flush();
			return output.getBytes();
		}

		public ByteResponseWrapper(HttpServletResponse response) {
			super(response);
			output = new ByteOutputStream();
			writer = new PrintWriter(output);
		}

		@Override
		public PrintWriter getWriter() {
			return writer;
		}

		@Override
		public ServletOutputStream getOutputStream() throws IOException {
			return output;
		}
	}

	static class ByteOutputStream extends ServletOutputStream {
		private ByteArrayOutputStream bos = new ByteArrayOutputStream();

		@Override
		public void write(int b) throws IOException {
			bos.write(b);
		}

		@Override
		public boolean isReady() {
			return false;
		}

		@Override
		public void setWriteListener(WriteListener arg0) {
		}
		
		public byte[] getBytes() {
			return bos.toByteArray();
		}
	}

}
